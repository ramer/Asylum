// Gate.h

#ifndef _GATE_h
#define _GATE_h

//GPIO #	Component
//GPIO00	Button1
//GPIO01	User
//GPIO02	User
//GPIO03	User
//GPIO04	Relay3
//GPIO05	Relay2
//GPIO09	Button2
//GPIO10	Button3
//GPIO12	Relay1
//GPIO13	Led1i
//GPIO14	Button4
//GPIO15	Relay4
//GPIO16	None
//FLAG	None

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "../Device.h"

#define debug(format, ...) Serial.printf_P((PGM_P)F(format), ## __VA_ARGS__)

#define INTERVAL_PINSET 500

class Gate : public Device
{
public:
  Gate(String id, String prefix, byte btnopen, byte open, byte btnclose, byte close);

  void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);

  void update();
  void updateState(ulong state_new);

protected:
  byte  pin_btnopen;
  byte  pin_open;
  byte  pin_btnclose;
  byte  pin_close;

  ulong pinsettime;
  bool pinset = false;

  Button btnopen;
  Button btnclose;
};

#endif