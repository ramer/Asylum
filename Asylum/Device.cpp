// Device.cpp

#include "Device.h"

Device::Device(String prefix, byte event, byte action) {
  uid_prefix = prefix;
  pin_event = event;
  pin_action = action;
}

Device::Device(String prefix) {
  uid_prefix = prefix;
}

Device::~Device() {}

void Device::initialize(PubSubClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  pinMode(pin_action, OUTPUT);

  generateUid();
  loadState();

  btn.begin(pin_event, true);
}

void Device::generateUid() {
  uint8_t mac[6]; WiFi.macAddress(mac);
  for (int i = sizeof(mac) - 2; i < sizeof(mac); ++i) uid_macsuffix += String(mac[i], HEX);
  uid = uid_prefix + "-" + uid_macsuffix;
  uid_filename = "/" + uid + ".json";
  mqtt_topic_sub = uid + "/pub";
  mqtt_topic_pub = uid + "/sub";
  html_control.replace("uid", uid);
}

void Device::update() {
  // process buttons
  if (btn.update() == DOWN) {
    debug(" - button down \n");
    invertState();
  }

  // check state saved
  if (!state_saved && millis() - state_savedtime > INTERVAL_STATE_SAVE) { saveState(); }

  // check state published
  if (_mqttClient && _mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state); }
}

void Device::updateState(ulong state_new) {
  state_last = state_new != state_off ? state_new : state_last;
  state = state_new;

  digitalWrite(pin_action, (state == state_off ? LOW : HIGH));

  state_saved = false;
  state_published = false;

  state_savedtime = millis();
  state_publishedtime = millis();

  debug(" - state changed to %u \n", state_new);
  //updateStateCallback(state_new);
}

void Device::invertState() {
  if (state == state_off) {
    if (state_last == state_off) { state_last = state_on; }
    updateState(state_last);
  }
  else {
    updateState(state_off);
  }
}

void Device::handlePayload(String topic, String payload) {
  if (topic == mqtt_topic_sub) {
    if (payload == "-1") {
      debug(" - value invert command recieved \n");

      invertState(); 
      publishState(mqtt_topic_pub, state); // force
    }
    else {
      ulong newvalue = payload.toInt();
      debug(" - value recieved: %u \n", newvalue);
	  debug("Free heap size: %u \n\n", ESP.getFreeHeap());

      updateState(newvalue); 
      publishState(mqtt_topic_pub, state); // force
    }
  }
}

void Device::subscribe() {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    _mqttClient->subscribe(mqtt_topic_sub.c_str());
  }
}

void Device::publishState(String topic, ulong statepayload) {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    _mqttClient->publish(topic.c_str(), String(statepayload).c_str(), true);

    debug(" - message sent [%s] %s \n", topic.c_str(), String(statepayload).c_str());

    state_published = true;
    state_publishedtime = millis();
  }
}

void Device::loadState() {
  if (!_config) return;
  byte onboot = _config->current["onboot"].toInt();
  if (onboot == 0) {
    // stay off
    debug(" - onboot: stay off \n");
    updateState(state_off);
  }
  else if (onboot == 1) {
    // turn on
    debug(" - onboot: turn on \n");
    updateState(state_on);
  }
  else if (onboot == 2) {
    // saved state
    std::map<String, String> states = _config->loadState(uid_filename);
    state = states["state"].toInt();
    state_last = states["state_last"].toInt();
    updateState(state);
    debug(" - onboot: last state: %u \n", state);
  }
  else if (onboot == 3) {
    // inverted saved state
    std::map<String, String> states = _config->loadState(uid_filename);
    state = states["state"].toInt();
    state_last = states["state_last"].toInt();
    invertState();
    debug(" - onboot: inverted state: %u \n", state);
    saveState();
  }
}

void Device::saveState() {
  if (!_config) return;
  byte onboot = _config->current["onboot"].toInt();
  if (onboot == 0 || onboot == 1) return;

  std::map<String, String> states;
  states.insert(std::pair<String, String>("state", String(state)));
  states.insert(std::pair<String, String>("state_last", String(state_last)));

  state_saved = true;
  state_savedtime = millis();

  _config->saveState(uid_filename, states);
}

//void Device::onUpdateState(std::function<void(ulong)> onUpdateStateCallback) {
//  updateStateCallback = onUpdateStateCallback;
//}
