// Strip.h

#ifdef ARDUINO_ESP8266_GENERIC

#ifndef _STRIP_h
#define _STRIP_h
#define USE_HSV

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "../Device.h"

#define INTERVAL_STRIP_UPDATE 	10
#define INTERVAL_STRIP_SOLID	  1000
#define INTERVAL_STRIP_RAINBOW	100
#define INTERVAL_STRIP_STARS	  100
#define INTERVAL_STRIP_SUNRISE	5000
#define INTERVAL_STRIP_SNAKE	  500

#define STRIP_LEDCOUNT          64 //121

#define STARS_PROBABILITY       400
#define STARS_INCREMENT         0x010101

class Strip : public Device
{
public:
  Strip(String prefix, byte strip_pin);

  void initialize(PubSubClient* ptr_mqttClient, Config* ptr_config);

  void update();

  void updateState(ulong state_new);

  uint32_t strip_wheel(byte angle);

  Adafruit_NeoPixel *strip;

protected:

  void frame_solid();
  void frame_rainbow();
  void frame_stars();
  void frame_sunrise();
	void frame_snake();

  ulong time_strip_update = 0;
  ulong time_strip_solid = 0;
  ulong time_strip_rainbow = 0;
  ulong time_strip_stars = 0;
	ulong time_strip_sunrise = 0;
	ulong time_strip_snake = 0;

  uint8_t rainbow_offset;
  uint8_t sunrise_offset;
	uint16_t snake_food = random(STRIP_LEDCOUNT);
	uint16_t snake = random(STRIP_LEDCOUNT);

	void increase(uint16_t* cur, uint16_t max);
	void decrease(uint16_t* cur, uint16_t max);
};

#endif

#endif