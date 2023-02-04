// BME280Sensor.cpp

#include "BME280Sensor.h"

BME280Sensor::BME280Sensor(String prefix, ulong interval) : Device(prefix) {
  state_publishedinterval = interval;

  html_control = "";
};

void BME280Sensor::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  generateUid();

  Wire.begin();
  if (!bme.begin()) {
    debug("Couldn't find BME280 \n");
  }
}

void BME280Sensor::update() {
  // check state published
  if (_mqttClient && _mqttClient->connected() && millis() - state_publishedtime > state_publishedinterval) {
    
    float temp, hum, pres;
    bme.read(pres, temp, hum, BME280::TempUnit_Celsius, BME280::PresUnit_torr);

    publishState(mqtt_topic_pub, temp, hum, pres);
  }
}

void BME280Sensor::publishState(String topic, float temp, float hum, float pres) {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    char payload[64];
    snprintf(payload, sizeof(payload), "{\"t\":\"%.1f\",\"h\":\"%.1f\",\"p\":\"%.1f\"}", temp, hum, pres);
    _mqttClient->publish(topic.c_str(), 1, true, payload);

    debug(" - message sent [%s] %s \n", topic.c_str(), payload);

    state_published = true;
    state_publishedtime = millis();
  }
}
