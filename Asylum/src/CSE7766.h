// CSE7766.h

#ifndef _CSE7766_h
#define _CSE7766_h

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "../Device.h"

#define CSE7766_SYNC_INTERVAL           300     // Safe time between transmissions (ms)
#define CSE7766_BAUDRATE                4800    // UART baudrate

#define CSE7766_V1R                     1.0     // 1mR current resistor
#define CSE7766_V2R                     1.0     // 1M voltage resistor

#define SENSOR_ERROR_OK             0       // No error
#define SENSOR_ERROR_OUT_OF_RANGE   1       // Result out of sensor range
#define SENSOR_ERROR_WARM_UP        2       // Sensor is warming-up
#define SENSOR_ERROR_TIMEOUT        3       // Response from sensor timed out
#define SENSOR_ERROR_UNKNOWN_ID     4       // Sensor did not report a known ID
#define SENSOR_ERROR_CRC            5       // Sensor data corrupted
#define SENSOR_ERROR_I2C            6       // Wrong or locked I2C address
#define SENSOR_ERROR_GPIO_USED      7       // The GPIO is already in use
#define SENSOR_ERROR_CALIBRATION    8       // Calibration error or Not calibrated
#define SENSOR_ERROR_OTHER          99      // Any other error

class CSE7766 : public Device
{
public:
  CSE7766(String id, String prefix, ulong interval);

  void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);

  void update();

  void publishState(String topic, uint32_t power, uint32_t voltage);

protected:
  ulong state_publishedinterval;
  ulong state_time;

  int error = 0;
  unsigned char data[24];

  double active = 0;
  double voltage = 0;
  double current = 0;
  double energy = 0;

  double ratioV = 1.0;
  double ratioC = 1.0;
  double ratioP = 1.0;

  bool checksum();
  void processData();
  void readData();
};

#endif

