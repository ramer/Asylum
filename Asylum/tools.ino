

void blynk(bool setup) {

#ifdef STATUS_LED
  pinMode(STATUS_LED, OUTPUT);

  if (setup)
  {
    if (millis() - status_led_time > STATUS_LED_INTERVAL) {
      status_led_time = millis();
      status_led_state = !status_led_state;
      digitalWrite(STATUS_LED, !status_led_state); // LED circuit inverted
    }
  }
  else {
    digitalWrite(STATUS_LED, (config.current["onboardled"].toInt() == 0 ? HIGH : LOW)); // LED circuit inverted
  }

#endif
}
