// Device.cpp

#include "Device.h"

Device::Device(String prefix, byte event, byte action) {
  uid_prefix = prefix;
  pin_event = event;
  pin_action = action;
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
}

void Device::update() {
  // process buttons
  if (btn.update() == DOWN) {
    debug(" - button down \n");
    invertState();
    saveState();
  }

  // check state published
  if (!_mqttClient) return;
  if (_mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state, &state_published); }
}

void Device::updateState(ulong state_new) {
  state_old = (state_new > 0 && state_old > 0) ? 0 : state_old;
  state = state_new;
  digitalWrite(pin_action, (state == 0 ? LOW : HIGH));
  state_published = false;

  debug(" - state changed to %u \n", state_new);
  //updateStateCallback(state_new);
}

void Device::invertState() {
  if (state == 0) {
    if (state_old == 0) { state_old = 1; }
    updateState(state_old);
  }
  else {
    updateState(0);
  }
}

void Device::handlePayload(String topic, String payload) {
  if (topic == mqtt_topic_sub) {
    if (payload == "-1") {
      debug(" - value invert command recieved \n");

      invertState(); //saveState();
      publishState(mqtt_topic_pub, state, &state_published); // force
    }
    else {
      ulong newvalue = payload.toInt();
      debug(" - value recieved: %u \n", newvalue);

      updateState(newvalue); //saveState();
      publishState(mqtt_topic_pub, state, &state_published); // force
    }
  }
}

void Device::subscribe() {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    _mqttClient->subscribe(mqtt_topic_sub.c_str());
  }
}

void Device::publishState(String topic, ulong statepayload, bool* ptr_state_published) {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    _mqttClient->publish(topic.c_str(), String(statepayload).c_str(), true);

    debug(" - message sent [%s] %s \n", topic.c_str(), String(statepayload).c_str());

    *ptr_state_published = true;
    state_publishedtime = millis();
  }
}

void Device::loadState() {
  if (!_config) return;
  byte onboot = _config->current["onboot"].toInt();
  if (onboot == 0) {
    // stay off
    debug(" - onboot: stay off \n");
    updateState(0);
  }
  else if (onboot == 1) {
    // turn on
    debug(" - onboot: turn on \n");
    updateState(1);
  }
  else if (onboot == 2) {
    // saved state
    std::map<String, String> states = _config->loadState(uid_filename);
    debug(" - onboot: last state: %u \n", states["state"].toInt());
    updateState(states["state"].toInt());
  }
  else if (onboot == 3) {
    // inverted saved state
    std::map<String, String> states = _config->loadState(uid_filename);
    if (states["on"].toInt() == 0) {
      debug(" - onboot: inverted state: %u \n", states["state"].toInt());
      updateState(states["state"].toInt());
      saveState();
    }
    else {
      debug(" - onboot: inverted state: 0 \n");
      updateState(0);
      saveState();
    }
  }
}

void Device::saveState() {
  if (!_config) return;
  byte onboot = _config->current["onboot"].toInt();
  if (onboot == 0 || onboot == 1) return;

  bool on = state;

  std::map<String, String> states_old = _config->loadState(uid_filename);
  std::map<String, String> states;

  states.insert(std::pair<String, String>("on", String((int)on)));

  if (on) {
    states.insert(std::pair<String, String>("state", String(state)));
  }
  else {
    states.insert(std::pair<String, String>("state", states_old["state"]));
  }

  _config->saveState(uid_filename, states);
}

//void Device::onUpdateState(std::function<void(ulong)> onUpdateStateCallback) {
//  updateStateCallback = onUpdateStateCallback;
//}
