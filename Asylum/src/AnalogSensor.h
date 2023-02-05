// AnalogSensor.h

#ifndef _ANALOGSENSOR_h
#define _ANALOGSENSOR_h

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "../Device.h"

#define PIN_AVERAGE_POW 10
#define INTERVAL_UPDATE 10

class AnalogSensor : public Device
{
public:
  AnalogSensor(String id, String prefix, byte event, ulong interval);

  void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);

  void update();

protected:
  ulong state_publishedinterval;
  ulong state_time;
};

#endif
