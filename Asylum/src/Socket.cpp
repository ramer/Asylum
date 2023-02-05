// Socket.cpp

#include "Socket.h"

Socket::Socket(String id, String prefix, byte event, byte action) : Device(id, prefix, event, action) {};

void Socket::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  Device::initialize(ptr_mqttClient, ptr_config);
}
