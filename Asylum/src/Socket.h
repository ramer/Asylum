// Socket.h

#ifndef _SOCKET_h
#define _SOCKET_h

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "../Device.h"

class Socket : public Device
{
public:
  Socket(String id, String prefix, byte event, byte action);

  void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);

protected:
};

#endif