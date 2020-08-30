// AnalogSensor.cpp

#include "AnalogSensor.h"

AnalogSensor::AnalogSensor(String prefix, byte event, ulong interval) : Device(prefix) {
  pin_event = event;
  state_publishedinterval = interval;

  html_control = "";
};

void AnalogSensor::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  pinMode(pin_event, INPUT);
  state = analogRead(pin_event);

  generateUid();
}

void AnalogSensor::update() {
  if (millis() - state_time > INTERVAL_UPDATE) {
    state_time = millis();
    state = (state * (PIN_AVERAGE_POW - 1) + (analogRead(pin_event))) / PIN_AVERAGE_POW;
  }

  // check state published
  if (_mqttClient && _mqttClient->connected() && millis() - state_publishedtime > state_publishedinterval) { publishState(mqtt_topic_pub, state); }
}