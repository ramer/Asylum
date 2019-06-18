// Button.h

#ifndef _BUTTON_h
#define _BUTTON_h

#include <ESP8266WiFi.h>

#define INTERVAL_UPDATE  5

enum ButtonState { STOPPED = -1, RELEASED = 0, DOWN = 1, PRESSED = 2, UP = 4 };

class Button
{
public:
  Button();
  Button(byte pin_number);

  void begin(bool inverted);
  void begin(byte pin_number, bool inverted);
  void stop();

  ButtonState update();
  ulong pressedTime();

protected:
  bool running = false;
  ulong update_time;

  byte         pin;
  bool         pin_inverted = false;
  ButtonState  pin_laststate = RELEASED;
  uint16_t     pin_average;
  ulong        pin_time;
};

#endif

