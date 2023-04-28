// EMeter.h

#ifndef _EMETER_h
#define _EMETER_h

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>
#include "../Device.h"

#define debug(format, ...) Serial.printf_P((PGM_P)F(format), ## __VA_ARGS__)

#include <TroykaOLED.h>
#include <RTClib.h>
#include <NtpClientLib.h>

#define IMPULSE 5 // 64

#define PIN_AVERAGE_POW 10
#define INTERVAL_UPDATE 10
#define INTERVAL_DISPLAY 10000

#define NTP_TIMEOUT 1500
#define NTP_SERVER "pool.ntp.org"
#define INTERVAL_UPDATE_NTP 5000

class EMeter : public Device
{
public:
  EMeter(String id, String prefix, byte event);

  void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);
  void generateTopics();
  void update();
  void handlePayload(String topic, String payload);
  void subscribe();

  String mqtt_topic_sub_e1;
  String mqtt_topic_sub_e2;
  String mqtt_topic_pub_e1;
  String mqtt_topic_pub_e2;

protected:
  TroykaOLED oled;
  RTC_DS1307 rtc;
  Ticker subscribeTimer;
  Ticker ntpTimer;

  ulong state_publishedinterval;
  ulong state_time;
  ulong display_time;

  uint8_t counter1, counter2 = 0;
  ulong e1 = 0;
  ulong e2 = 0;

  ulong min = 0;
  ulong max = 1024;

  bool lastState;

  bool ntp_started = false;
  int8_t ntp_timezone = 3;
  int8_t ntp_timezoneminutes = 0;
  time_t ntp_lastsync = 0;
  unsigned long ntp_time = 0;

  void subscribeDelayed();
  void unsubscribeDelayed();

  bool closerToHigh(int val, int lowval, int highval);
  void displayState();
  void increaseCounter();

  bool getNtp(RTC_DS1307* dt);
};

#endif
