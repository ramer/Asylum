// SHT31.cpp.h
// Based on
/****************************************************************************/
//  Function:       Header file for TroykaMeteoSensor
//  Hardware:       SHT30, SHT31 and SHT35
//  Arduino IDE:    Arduino-1.8.5
//  Author:         Igor Dementiev
//  Date:           Aug 20,2018
//  Version:        v1.0.0
//  by www.amperka.ru
/****************************************************************************/

#if (defined DEVICE_TYPE_SHT31SENSOR)

#ifndef _SHT31_cpp_h
#define _SHT31_cpp_h

#include <Wire.h>

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "../Device.h"

#define JUMPER_OFF                      0x44
#define JUMPER_ON                       0x45

#define SHT_REP_HIGH_CLOCK_STRETCH      0x2C06
#define SHT_REP_MEDIUM_CLOCK_STRETCH    0x2C0D
#define SHT_REP_LOW_CLOCK_STRETCH       0x2C10
#define SHT_REP_HIGH                    0x2400
#define SHT_REP_MEDIUM                  0x240B
#define SHT_REP_LOW                     0x2416
#define SHT_HEATER_ON                   0x306D
#define SHT_HEATER_OFF                  0x3066
#define SHT_SOFT_RESET                  0x30A2
#define SHT_READ_STATUS                 0xF32D

#define SHT_OK                          0
#define SHT_ERROR_DATA                  -1
#define SHT_ERROR_CHECKSUM              -2

#define SHT_DATA_SIZE                   6
#define SHT_CELSIUS_TO_KELVIN           273.15

class SHT31 : public Device
{
public:
  SHT31(String prefix, ulong interval, int i2cAddr = JUMPER_OFF);

  void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);

  void update();

  void publishState(String topic, float temp, float hum);

  void begin();
  void reset();
  int8_t read();
  void setRepeatability(uint8_t repeatability);
  void clockStretchingOn();
  void clockStretchingOff();
  void heaterOn();
  void heaterOff();
  float getTemperature() const { return _temperature; }
  float getHumidity() const { return _humidity; }

protected:
  ulong state_publishedinterval;
  ulong state_time;

  void _writeReg(uint16_t data);
  uint8_t _checkCRC8(const uint8_t* data, int len);
  int _i2cAddr;
  uint8_t _repeatability;
  bool _stateClockStretching;
  float _temperature;
  float _humidity;
};

SHT31::SHT31(String prefix, ulong interval, int i2cAddr) : Device(prefix) {
  state_publishedinterval = interval;

  html_control = "";

  _i2cAddr = i2cAddr;
};

void SHT31::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  generateUid();

  begin();
}

void SHT31::update() {
  // check state published
  if (_mqttClient && _mqttClient->connected() && millis() - state_publishedtime > state_publishedinterval) {

    int stateSensor = read();
    float temp, hum;

    switch (stateSensor) {
    case SHT_OK:
      temp = getTemperature();
      hum = getHumidity();
      break;
    case SHT_ERROR_DATA:
      temp = NAN;
      hum = NAN;
      debug("SHT31 Data error or sensor not connected");
      break;
    case SHT_ERROR_CHECKSUM:
      temp = NAN;
      hum = NAN;
      debug("SHT31 Checksum error");
      break;
    }

    publishState(mqtt_topic_pub, temp, hum);
  }
}

void SHT31::publishState(String topic, float temp, float hum) {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    char payload[64];
    snprintf(payload, sizeof(payload), "{\"t\":\"%.1f\",\"h\":\"%.1f\"}", temp, hum);
    _mqttClient->publish(topic.c_str(), 1, true, payload);

    debug(" - message sent [%s] %s \n", topic.c_str(), payload);

    state_published = true;
    state_publishedtime = millis();
  }
}



enum { HIGH_STATE, MEDIUM_STATE, LOW_STATE };

void SHT31::begin() {
  Wire.begin();
  setRepeatability(HIGH_STATE);
  clockStretchingOff();
}

void SHT31::reset() {
  _writeReg(SHT_SOFT_RESET);
}

int8_t SHT31::read() {
  switch (_repeatability) {
  case HIGH_STATE:
    if (_stateClockStretching) {
      _writeReg(SHT_REP_HIGH_CLOCK_STRETCH);
    }
    else {
      _writeReg(SHT_REP_HIGH);
    }
    break;
  case MEDIUM_STATE:
    if (_stateClockStretching) {
      _writeReg(SHT_REP_MEDIUM_CLOCK_STRETCH);
    }
    else {
      _writeReg(SHT_REP_MEDIUM);
    }
    break;
  case LOW_STATE:
    if (_stateClockStretching) {
      _writeReg(SHT_REP_LOW_CLOCK_STRETCH);
    }
    else {
      _writeReg(SHT_REP_LOW);
    }
    break;
  }

  uint8_t data[SHT_DATA_SIZE];

  Wire.requestFrom(_i2cAddr, SHT_DATA_SIZE);

  if (Wire.available() != SHT_DATA_SIZE) {
    return SHT_ERROR_DATA;
  }

  for (int i = 0; i < SHT_DATA_SIZE; i++) {
    data[i] = Wire.read();
  }

  if (data[2] != _checkCRC8(data, 2) || data[5] != _checkCRC8(data + 3, 2)) {
    return SHT_ERROR_CHECKSUM;
  }

  _temperature = ((((data[0] * 256.0) + data[1]) * 175.0) / 65535.0) - 45.0;
  _humidity = ((((data[3] * 256.0) + data[4]) * 100.0) / 65535.0);

  return SHT_OK;
}

void SHT31::setRepeatability(uint8_t repeatability) {
  switch (repeatability) {
  case HIGH_STATE:
    _repeatability = HIGH_STATE;
    break;
  case MEDIUM_STATE:
    _repeatability = MEDIUM_STATE;
    break;
  case LOW_STATE:
    _repeatability = LOW_STATE;
    break;
  default:
    _repeatability = HIGH_STATE;
    break;
  }
}

void SHT31::clockStretchingOn() {
  _stateClockStretching = true;
}

void SHT31::clockStretchingOff() {
  _stateClockStretching = false;
}

void SHT31::heaterOn() {
  _writeReg(SHT_HEATER_ON);
}

void SHT31::heaterOff() {
  _writeReg(SHT_HEATER_OFF);
}

void SHT31::_writeReg(uint16_t data) {
  Wire.beginTransmission(_i2cAddr);
  Wire.write(data >> 8);
  Wire.write(data & 0xFF);
  Wire.endTransmission();
}

uint8_t SHT31::_checkCRC8(const uint8_t* data, int len) {
  // CRC-8 formula from page 14 of SHT Datasheet
  const uint8_t polynomial = 0x31;
  uint8_t crc = 0xFF;
  for (uint8_t byte = 0; byte < len; byte++) {
    crc ^= data[byte];
    for (int bit = 0; bit < 8; bit++) {
      crc = (crc & 0x80) ? (crc << 1) ^ polynomial : (crc << 1);
    }
  }
  return crc;
}

#endif
#endif

