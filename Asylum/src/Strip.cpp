// Strip.cpp

#ifdef ARDUINO_ESP8266_GENERIC

#include "Strip.h"

Strip::Strip(String prefix, byte action) : Device(prefix) {
  pin_action = action;

  state_on = 0xFFFFFF;
  state_off = 0;

  html_control = R"~(
    <div class="field-group">
        <input id="uid" type="color" onchange="onColorChange(event)" class="toggle" value="#ffffff">
            <label for="uid"></label>
        </input>
    </div>)~";
};

void Strip::initialize(PubSubClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  pinMode(pin_action, OUTPUT);

  //strip = Adafruit_NeoPixel(STRIP_LEDCOUNT, pin_action, NEO_GRB + NEO_KHZ800);
  //strip.begin();
  //strip.show();

  generateUid();
  loadState();
}

void Strip::update() {
  // update state of strip
  update_strip();

  // check state saved
  if (!state_saved && millis() - state_savedtime > INTERVAL_STATE_SAVE) { saveState(); }

  // check state published
  if (_mqttClient && _mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state); }
}

void Strip::updateState(ulong state_new) {
  state_last = state_new != state_off ? state_new : state_last;
  state = state_new;

  state_saved = false;
  state_published = false;

  state_savedtime = millis();
  state_publishedtime = millis();

  debug(" - state changed to %u \n", state_new);
}

void Strip::update_strip() {
  if (millis() - time_strip_update > INTERVAL_STRIP_UPDATE) {
    time_strip_update = millis();

    switch (state) {
    case 1:
      frame_rainbow();
      break;
    case 2:
      frame_stars();
      break;
    case 3:
      frame_sunrise();
      break;
    default:
      frame_solid();
      break;
    }
  }
}

void Strip::frame_solid() {
  if (millis() - time_strip_solid > INTERVAL_STRIP_SOLID) {
    time_strip_solid = millis();
    
    uint8_t r = state >> 16;
    uint8_t g = state >> 8 & 0xFF;
    uint8_t b = state & 0xFF;

    //for (int i = 0; i < strip.numPixels(); i++) {
    //  strip.setPixelColor(i, strip.Color(r, g, b));
    //}

    //strip.show();
    sunrise_offset = 0; // prevent sunrise start from the middle
  }
}

void Strip::frame_rainbow() {
  if (millis() - time_strip_rainbow > INTERVAL_STRIP_RAINBOW) {
    time_strip_rainbow = millis();

    //for (uint16_t i = 0; i < strip.numPixels(); i++) {
    //  strip.setPixelColor(i, strip_wheel((((i + rainbow_offset) * 256 / strip.numPixels())) & 255));
    //}

    //strip.show();
    rainbow_offset++;
  }
}

void Strip::frame_stars() {
  if (millis() - time_strip_stars > INTERVAL_STRIP_STARS) {
    time_strip_stars = millis();

    //if (random(STARS_PROBABILITY) == 0) { stars[random(strip.numPixels() - 1)] = STARS_INCREMENT; }

    //for (uint16_t i = 0; i < strip.numPixels(); i++) {
    //  if (stars[i] > 0) { stars[i] = stars[i] + STARS_INCREMENT; }
    //}

    //for (uint16_t i = 0; i < strip.numPixels(); i++) {
    //  strip.setPixelColor(i, strip.Color(stars[i], stars[i], stars[i]));
    //}
    //strip.show();
  }
}

void Strip::frame_sunrise() {
  if (millis() - time_strip_sunrise > INTERVAL_STRIP_SUNRISE) {
    time_strip_sunrise = millis();

    //for (uint16_t i = 0; i < strip.numPixels(); i++) {
    //  strip.setPixelColor(i, strip.Color(sunrise_offset, sunrise_offset, sunrise_offset));
    //}

    //if (sunrise_offset >= 255) {
    //  sunrise_offset = 0;
    //  updateState(0xffffff);
    //}

    //strip.show();
    sunrise_offset++;
  }
}

//uint32_t Strip::strip_wheel(byte angle) {
//  angle = 255 - angle;
//  if (angle < 85) {
//    return strip.Color(255 - angle * 3, 0, angle * 3);
//  }
//  if (angle < 170) {
//    angle -= 85;
//    return strip.Color(0, angle * 3, 255 - angle * 3);
//  }
//  angle -= 170;
//  return strip.Color(angle * 3, 255 - angle * 3, 0);
//}

#endif