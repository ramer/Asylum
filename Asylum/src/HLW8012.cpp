// HLW8012.cpp

#include "HLW8012.h"

HLW8012::HLW8012(String id, String prefix, byte pin_pwr, byte pin_vltcur, byte pin_sw , ulong interval) : Device(id, prefix) {
  pin_event = pin_pwr;
  pin_voltagecurrent = pin_vltcur;
  pin_switch = pin_sw;
  state_publishedinterval = interval;

  html_control = "";
};

void HLW8012::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  generateTopics();

  sonoffpowinstance = this;

  pinMode(pin_switch, OUTPUT);
  digitalWrite(pin_switch, LOW);

  pinMode(pin_event, INPUT_PULLUP);
  attachInterrupt(pin_event, PowerInterruptFunc, FALLING);

  pinMode(pin_voltagecurrent, INPUT_PULLUP);
  attachInterrupt(pin_voltagecurrent, VoltageInterruptFunc, FALLING);

  os_timer_setfn(&timer, (os_timer_func_t*)TimerFunc, NULL);
  os_timer_arm(&timer, 1000, true);  // every ms
}

void HLW8012::update() {
  // check state published
  if (_mqttClient && _mqttClient->connected() && millis() - state_publishedtime > state_publishedinterval) { 
    publishState(mqtt_topic_pub, frequency_power * MLT_POWER, frequency_voltage * MLT_VOLTAGE);
  }
}

void HLW8012::doPower() {
  frequency_power_counter++;
}

void HLW8012::doVoltage() {
  frequency_voltage_counter++;
}

void HLW8012::doTimer(void)
{
  frequency_power = frequency_power_counter;
  frequency_power_counter = 0;

  frequency_voltage = frequency_voltage_counter;
  frequency_voltage_counter = 0;
}

void HLW8012::publishState(String topic, uint32_t power, uint32_t voltage) {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    char payload[64];
    snprintf(payload, sizeof(payload), "{\"w\":\"%u\",\"v\":\"%u\"}", power, voltage);
    _mqttClient->publish(topic.c_str(), 1, true, payload);

    debug(" - message sent [%s] %s \n", topic.c_str(), payload);

    state_published = true;
    state_publishedtime = millis();
  }
}
