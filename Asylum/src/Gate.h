// Gate.h

#ifndef _GATE_h
#define _GATE_h

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "../Device.h"

#define debug(format, ...) Serial.printf_P((PGM_P)F(format), ## __VA_ARGS__)

#define INTERVAL_PINSET		    500

class Gate : public Device
{
public:
  Gate(String id, String prefix, byte event, byte action);

  void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);

  void update();
  void updateState(ulong state_new);

protected:
  byte  pin_open;
  byte  pin_close;

  ulong pinsettime;
  bool pinset = false;
};

#endif