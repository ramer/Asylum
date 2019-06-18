// Button.cpp

#include "Button.h"

Button::Button() {}

Button::Button(byte pin_number) {
  pin = pin_number;
}

void Button::begin(bool inverted = false) {
  pinMode(pin, INPUT);
  pin_inverted = inverted;
  pin_average = digitalRead(pin) ^ pin_inverted;  // initial value
  running = true;
}

void Button::begin(byte pin_number, bool inverted = false) {
  pin = pin_number;
  pinMode(pin, INPUT);
  pin_inverted = inverted;
  pin_average = digitalRead(pin) ^ pin_inverted;  // initial value
  running = true;
}

void Button::stop() {
  running = false;
}

ButtonState Button::update()
{
  if (!running) return STOPPED;
  if (!(millis() - update_time > INTERVAL_UPDATE)) {
    if (pin_laststate == DOWN) pin_laststate = PRESSED;
    return pin_laststate;
  }

  update_time = millis();

  // avg      = (             avg * 3             +             0 or 255                    + 2) /  4
  pin_average = ((pin_average << 2) - pin_average + (digitalRead(pin) ^ pin_inverted) * 255 + 2) >> 2; // for uint16
  // pin_average = (pin_average * (PIN_AVERAGE - 1) + (digitalRead(pin) ^ pin_inverted)) / PIN_AVERAGE; // for float

  if (pin_laststate == RELEASED && pin_average > 223)
  {
    pin_time = millis();
    pin_laststate = DOWN;
    return pin_laststate;
  }

  if (pin_laststate == DOWN)
  {
    pin_laststate = PRESSED;
    return pin_laststate;
  }

  if ((pin_laststate == DOWN || pin_laststate == PRESSED) && pin_average < 32)
  {
    pin_laststate = UP;
    return pin_laststate;
  }

  if (pin_laststate == UP && pin_average < 32)
  {
    pin_laststate = RELEASED;
    return pin_laststate;
  }

  return pin_laststate;
}

ulong Button::pressedTime()
{
  return pin_laststate == PRESSED ? millis() - pin_time : 0;
}