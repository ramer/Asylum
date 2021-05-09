// CSE7766.cpp

#include "CSE7766.h"

CSE7766::CSE7766(String prefix, ulong interval) : Device(prefix) {
  state_publishedinterval = interval;

  html_control = "";
};

void CSE7766::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  generateUid();

  Serial.begin(CSE7766_BAUDRATE);

}

void CSE7766::update() {
  // check state published
  if (_mqttClient && _mqttClient->connected() && millis() - state_publishedtime > state_publishedinterval) {
    readData();
    publishState(mqtt_topic_pub, active, voltage);
  }
}

void CSE7766::publishState(String topic, uint32_t power, uint32_t voltage) {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    char payload[64];
    snprintf(payload, sizeof(payload), "{\"w\":\"%u\",\"v\":\"%u\"}", power, voltage);
    _mqttClient->publish(topic.c_str(), 1, true, payload);

    debug(" - message sent [%s] %s \n", topic.c_str(), payload);

    state_published = true;
    state_publishedtime = millis();
  }
}

void CSE7766::readData() {

  error = SENSOR_ERROR_OK;

  static unsigned char index = 0;
  static unsigned long last = millis();

  while (Serial.available()) {

    // A 24 bytes message takes ~55ms to go through at 4800 bps
    // Reset counter if more than 1000ms have passed since last byte.
    if (millis() - last > CSE7766_SYNC_INTERVAL) index = 0;
    last = millis();

    uint8_t byte = Serial.read();

    // first byte must be 0x55 or 0xF?
    if (0 == index) {
      if ((0x55 != byte) && (byte < 0xF0)) {
        continue;
      }

      // second byte must be 0x5A
    }
    else if (1 == index) {
      if (0x5A != byte) {
        index = 0;
        continue;
      }
    }

    data[index++] = byte;
    if (index > 23) {
      Serial.flush();
      break;
    }

  }

  // Process packet
  if (24 == index) {
    processData();
    index = 0;
  }
}

void CSE7766::processData() {

  // Sample data:
  // 55 5A 02 E9 50 00 03 31 00 3E 9E 00 0D 30 4F 44 F8 00 12 65 F1 81 76 72 (w/ load)
  // F2 5A 02 E9 50 00 03 2B 00 3E 9E 02 D7 7C 4F 44 F8 CF A5 5D E1 B3 2A B4 (w/o load)

  // Checksum
  if (!checksum()) {
    error = SENSOR_ERROR_CRC;
    return;
  }

  // Calibration
  if (0xAA == data[0]) {
    error = SENSOR_ERROR_CALIBRATION;
    return;
  }

  if ((data[0] & 0xFC) > 0xF0) {
    error = SENSOR_ERROR_OTHER;
    return;
  }

  // Calibration coefficients
  unsigned long _coefV = (data[2] << 16 | data[3] << 8 | data[4]);              // 190770
  unsigned long _coefC = (data[8] << 16 | data[9] << 8 | data[10]);             // 16030
  unsigned long _coefP = (data[14] << 16 | data[15] << 8 | data[16]);           // 5195000

  // Adj: this looks like a sampling report
  uint8_t adj = data[20];                                                       // F1 11110001

  // Calculate voltage
  voltage = 0;
  if ((adj & 0x40) == 0x40) {
    unsigned long voltage_cycle = data[5] << 16 | data[6] << 8 | data[7];       // 817
    voltage = ratioV * _coefV / voltage_cycle / CSE7766_V2R;                    // 190700 / 817 = 233.41
  }

  // Calculate power
  active = 0;
  if ((adj & 0x10) == 0x10) {
    if ((data[0] & 0xF2) != 0xF2) {
      unsigned long power_cycle = data[17] << 16 | data[18] << 8 | data[19];     // 4709
      active = ratioP * _coefP / power_cycle / CSE7766_V1R / CSE7766_V2R;        // 5195000 / 4709 = 1103.20
    }
  }

  // Calculate current
  current = 0;
  if ((adj & 0x20) == 0x20) {
    if (active > 0) {
      unsigned long current_cycle = data[11] << 16 | data[12] << 8 | data[13];   // 3376
      current = ratioC * _coefC / current_cycle / CSE7766_V1R;                   // 16030 / 3376 = 4.75
    }
  }

  // Calculate energy
  unsigned int difference;
  static unsigned int cf_pulses_last = 0;
  unsigned int cf_pulses = data[21] << 8 | data[22];
  if (0 == cf_pulses_last) cf_pulses_last = cf_pulses;
  if (cf_pulses < cf_pulses_last) {
    difference = cf_pulses + (0xFFFF - cf_pulses_last) + 1;
  }
  else {
    difference = cf_pulses - cf_pulses_last;
  }
  energy += difference * (float)_coefP / 1000000.0;
  cf_pulses_last = cf_pulses;
}

bool CSE7766::checksum() {
  unsigned char checksum = 0;
  for (unsigned char i = 2; i < 23; i++) {
    checksum += data[i];
  }
  return checksum == data[23];
}