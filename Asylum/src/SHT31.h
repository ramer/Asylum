// SHT31.h
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

#ifndef _SHT31_h
#define _SHT31_h

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


#endif

