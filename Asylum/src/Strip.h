// Strip.h

#ifdef ARDUINO_ESP8266_GENERIC

#ifndef _STRIP_h
#define _STRIP_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "../Device.h"

#define INTERVAL_STRIP_UPDATE 	10
#define INTERVAL_STRIP_RAINBOW	100
#define INTERVAL_STRIP_STARS	  10
#define INTERVAL_STRIP_SUNRISE	1000
#define INTERVAL_STRIP_SOLID	  1000

#define STRIP_LEDCOUNT          121
#define STARS_PROBABILITY       200
#define STARS_INCREMENT         1

class Strip : public Device
{
public:
  Strip(String prefix, byte action);

  void initialize(PubSubClient* ptr_mqttClient, Config* ptr_config);

  void update();

  void updateState(ulong state_new);


protected:

  void update_strip();
  void frame_solid();
  void frame_rainbow();
  void frame_stars();
  void frame_sunrise();
  uint32_t strip_wheel(byte angle);

  ulong time_strip_update = 0;
  ulong time_strip_rainbow = 0;
  ulong time_strip_stars = 0;
  ulong time_strip_sunrise = 0;
  ulong time_strip_solid = 0;

  uint8_t rainbow_offset;
  uint8_t stars[STRIP_LEDCOUNT];
  uint8_t sunrise_offset;

  Adafruit_NeoPixel strip;
};

#endif

#endif