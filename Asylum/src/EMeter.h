// EMeter.h

#ifndef _EMETER_h
#define _EMETER_h

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "../Device.h"

#define PIN_AVERAGE_POW 10
#define INTERVAL_UPDATE 10

class EMeter : public Device
{
public:
  EMeter(String id, String prefix, byte event);

  void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);

  void update();

protected:
  ulong state_publishedinterval;
  ulong state_time;
};

#endif
