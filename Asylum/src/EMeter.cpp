// EMeter.cpp

#include "EMeter.h"

EMeter::EMeter(String id, String prefix, byte event) : Device(id, prefix) {
  pin_event = event;
  //state_publishedinterval = interval;

  html_control = "";
};

void EMeter::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  pinMode(pin_event, INPUT);
  state = analogRead(pin_event);
  lastState = closerToHigh(state, min, max);

  generateTopics();

  debug(" - initializing RTC ... ");
  if (rtc.begin()) {
    if (!rtc.isrunning()) {
      debug("NOT running, adjusting upload time. \n");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    else {
      DateTime datetime = rtc.now();
      debug("done: %02u:%02u:%02u \n", datetime.hour(), datetime.minute(), datetime.second());
    }
  }
  else {
    debug("failure \n");
  }

  debug(" - initializing OLED. \n");
  oled.begin();

  ntpTimer.attach(10, { std::bind(&EMeter::subscribeDelayed, this) });

  displayState();
}

void EMeter::generateTopics() {
  uid_filename = uid + "-" + uid_prefix + ".json";
  mqtt_topic_sub_e1 = uid + "/" + uid_prefix + "/sub/e1";
  mqtt_topic_sub_e2 = uid + "/" + uid_prefix + "/sub/e2";
  mqtt_topic_pub_e1 = uid + "/" + uid_prefix + "/pub/e1";
  mqtt_topic_pub_e2 = uid + "/" + uid_prefix + "/pub/e2";

  html_control.replace("uid", uid);
}

void EMeter::handlePayload(String topic, String payload) {
  if (topic == mqtt_topic_sub_e1) {
    e1 = payload.toInt();
    debug(" - E1 value recieved: %u \n", e1);
  }
  if (topic == mqtt_topic_sub_e2) {
    e2 = payload.toInt();
    debug(" - E2 value recieved: %u \n", e2);
  }
  if (topic == mqtt_topic_pub_e1) {
    e1 = payload.toInt();
    debug(" - E1 value recieved: %u \n", e1);
  }
  if (topic == mqtt_topic_pub_e2) {
    e2 = payload.toInt();
    debug(" - E2 value recieved: %u \n", e2);
  }
  displayState();
}

void EMeter::subscribe() {
  subscribeTimer.once(5, { std::bind(&EMeter::subscribeDelayed, this) });
}

void EMeter::subscribeDelayed() {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    _mqttClient->subscribe(mqtt_topic_sub_e1.c_str(), 0);
    _mqttClient->subscribe(mqtt_topic_sub_e2.c_str(), 0);
    _mqttClient->subscribe(mqtt_topic_pub_e1.c_str(), 0);
    _mqttClient->subscribe(mqtt_topic_pub_e2.c_str(), 0);

    _mqttClient->unsubscribe(mqtt_topic_pub_e1.c_str());
    _mqttClient->unsubscribe(mqtt_topic_pub_e2.c_str());
  }
}

void EMeter::update() {
  if (millis() - state_time > INTERVAL_UPDATE) {
    state_time = millis();

    state = analogRead(pin_event);

    if (closerToHigh(state, min, max)) {
      max = (max * (PIN_AVERAGE_POW - 1) + (state)) / PIN_AVERAGE_POW;
      if (lastState == LOW) {
        lastState = HIGH;
        increaseCounter();
      }
    }
    else {
      min = (min * (PIN_AVERAGE_POW - 1) + (state)) / PIN_AVERAGE_POW;
      if (lastState == HIGH) {
        lastState = LOW;
      }
    }
  }

  //if (millis() - display_time > INTERVAL_DISPLAY) {
  //  displayState();
  //}

  if (millis() - ntp_time > INTERVAL_UPDATE_NTP) {
    ntp_time = millis();
    getNtp(&rtc);
  }

  // check state published
  if (_mqttClient && _mqttClient->connected() && millis() - state_publishedtime > state_publishedinterval) { publishState(mqtt_topic_pub, state); }
}

bool EMeter::closerToHigh(int val, int lowval, int highval) {
  int middle = (highval - lowval) / 2 + lowval;
  return val > middle;
}

void EMeter::displayState() {
  display_time = millis();

  oled.drawRect(31, 10, 128, 56, true, BLACK);

  oled.setFont(alphabet6x8);
  oled.print("E1", 0, 15);
  oled.print("E2", 0, 45);

  oled.setFont(alphabet12x16);

  String s1 = String(e1 / 10) + "." + String(e1 % 10);
  String s2 = String(e2 / 10) + "." + String(e2 % 10);

  oled.print(s1, 31, 10);
  oled.print(s2, 31, 40);
}

void EMeter::increaseCounter() {
  DateTime dt = rtc.now();
  if (dt.hour() >= 7 && dt.hour() < 23) {
    counter1++;
    if (counter1 >= IMPULSE) {
      e1++;
      counter1 = 0;
      publishState(mqtt_topic_pub_e1, e1);
      displayState();
    }
  }
  else {
    counter2++;
    if (counter2 >= IMPULSE) {
      e2++;
      counter2 = 0;
      publishState(mqtt_topic_pub_e2, e2);
      displayState();
    }
  }
}

bool EMeter::getNtp(RTC_DS1307* rtci) {
  if (WiFi.status() == WL_CONNECTED && !ntp_started) {
    debug(" - initializing NTP ... ");
    NTP.setNTPTimeout(NTP_TIMEOUT);
    if (NTP.begin(NTP_SERVER, ntp_timezone, false, ntp_timezoneminutes)) {
      ntp_started = true;
      debug("done: \n");
    }
    else {
      debug("failed \n");
    }
  }

  if (WiFi.status() == WL_DISCONNECTED && ntp_started) {
    debug(" - deinitializing NTP ... ");
    if (NTP.stop()) {
      ntp_started = false;
      debug("done: \n");
    }
    else {
      debug("failed \n");
    }
  }

  if (NTP.getLastNTPSync() > ntp_lastsync) {
    ntp_lastsync = NTP.getLastNTPSync();
    rtci->adjust(DateTime(now() + 2)); // now is current time from time library, updated by NTP. 2 secs added to compensate RTC adjust time
    debug(" - NTP sync: %s \n", NTP.getTimeDateString().c_str());
  }
}