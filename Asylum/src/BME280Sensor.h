// BME280Sensor.h

#ifndef _BME280Sensor_h
#define _BME280Sensor_h

#include <Wire.h>
#include <BME280I2C.h>

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "../Device.h"

#define debug(format, ...) Serial.printf_P((PGM_P)F(format), ## __VA_ARGS__)

class BME280Sensor : public Device
{
public:
  BME280Sensor(String id, String prefix, ulong interval);

  void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);

  void update();

  void publishState(String topic, float temp, float hum, float pres);

protected:
  ulong state_publishedinterval;
  ulong state_time;

  BME280I2C bme;
};

#endif

