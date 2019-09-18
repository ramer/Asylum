

void blynk(bool setup) {

#ifdef STATUS_LED
  pinMode(STATUS_LED, OUTPUT);

  if (setup)
  {
    if (millis() - time_status_led > INTERVAL_STATUS_LED) {
      time_status_led = millis();
      state_status_led = !state_status_led;
      digitalWrite(STATUS_LED, !state_status_led); // LED circuit inverted
    }
  }
  else {
    digitalWrite(STATUS_LED, (config.current["onboardled"].toInt() == 0 ? HIGH : LOW)); // LED circuit inverted
  }

#endif
}
