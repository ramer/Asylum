// Socket.cpp

#include "Socket.h"

Socket::Socket(String prefix, byte event, byte action) : Device(prefix, event, action) {};

void Socket::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  Device::initialize(ptr_mqttClient, ptr_config);
}
