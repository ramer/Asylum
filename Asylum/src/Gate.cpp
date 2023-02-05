// Gate.cpp

#include "Gate.h"

Gate::Gate(String id, String prefix, byte open, byte close) : Device(id, prefix) {
  uid_prefix = prefix;
  pin_open = open;
  pin_close = close;
};

void Gate::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  pinMode(pin_open, OUTPUT);
  pinMode(pin_close, OUTPUT);

  digitalWrite(pin_open, LOW);
  digitalWrite(pin_close, LOW);

  generateTopics();
  loadState();
}

void Gate::update() {
  // check state saved
  if (!state_saved && millis() - state_savedtime > INTERVAL_STATE_SAVE) { saveState(); }

  // check pinset
  if (pinset && millis() - pinsettime > INTERVAL_PINSET) { 
    pinset = false;
    digitalWrite(pin_open, LOW);
    digitalWrite(pin_close, LOW);
  }

  // check state published
  if (_mqttClient && _mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state); }
}

void Gate::updateState(ulong state_new) {
  state_last = state_new != state_off ? state_new : state_last;
  state = state_new;

  if (state == state_off) {
    digitalWrite(pin_close, HIGH);
    debug(" - close inpulse sent\n");
  }
  else {
    digitalWrite(pin_open, HIGH);
    debug(" - open inpulse sent\n");
  }
  pinsettime = millis();
  pinset = true;

  state_saved = false;
  state_published = false;

  state_savedtime = millis();
  state_publishedtime = millis();

  debug(" - state changed to %u \n", state_new);
  //updateStateCallback(state_new);
}