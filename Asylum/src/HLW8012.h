// HLW8012.h

#ifndef _HLW8012_h
#define _HLW8012_h

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "../Device.h"

#define MLT_POWER   10.9842
#define MLT_VOLTAGE 0.452
#define MLT_CURRENT 1

class HLW8012 : public Device
{
public:
  HLW8012(String id, String prefix, byte pin_pwr, byte pin_vltcur, byte pin_sw, ulong interval);

  void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);

  void update();

  void doPower();
  void doVoltage();
  void doTimer();

  void publishState(String topic, uint32_t power, uint32_t voltage);

protected:
  uint32_t frequency_power_counter = 0;
  uint32_t frequency_power = 0;
  uint32_t frequency_voltage_counter = 0;
  uint32_t frequency_voltage = 0;

  os_timer_t timer;

  byte pin_voltagecurrent;
  byte pin_switch;

  ulong state_publishedinterval;
  ulong state_time;
};

static HLW8012* sonoffpowinstance;
ICACHE_RAM_ATTR static void PowerInterruptFunc() {
  sonoffpowinstance->doPower();
}
ICACHE_RAM_ATTR static void VoltageInterruptFunc() {
  sonoffpowinstance->doVoltage();
}
ICACHE_RAM_ATTR static void TimerFunc() {
  sonoffpowinstance->doTimer();
}

#endif

