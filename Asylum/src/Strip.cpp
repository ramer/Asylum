// strip->cpp

#if (defined DEVICE_TYPE_STRIP)

#include "strip.h"

Strip::Strip(String prefix, byte strip_pin) : Device(prefix) {
	strip = new Adafruit_NeoPixel(STRIP_LEDCOUNT, strip_pin, NEO_BRG + NEO_KHZ400);

  state_on = 0xFFFFFF;
  state_off = 0;

  html_control = R"~(
    <div class="field-group">
        <input id="uid" type="color" onchange="onColorChange(event)" class="toggle" value="#ffffff">
            <label for="uid"></label>
        </input>
    </div>)~";
};

void Strip::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  pinMode(pin_action, OUTPUT);

  strip->begin();
  strip->show();

  generateTopics();
  loadState();
}

void Strip::update() {
  // update state of strip
	if (millis() - time_strip_update > INTERVAL_STRIP_UPDATE) {
		time_strip_update = millis();

		if (state == 1) {
			frame_rainbow();
		}
		else if (state == 2) {
			frame_stars();
		}
		else if (state == 3) {
			frame_sunrise();
		}
		else if (state == 4) {
			frame_snake();
		}
		else {
			frame_solid();
		}
	}

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

	// update instantly
	time_strip_update = 0;
	time_strip_rainbow = 0;
	time_strip_stars = 0;
	time_strip_sunrise = 0;
	time_strip_solid = 0;

  solid_updated = false;
  sunrise_offset = 0;

  debug(" - state changed to %u \n", state_new);
}

void Strip::frame_solid() {
  if (millis() - time_strip_solid > INTERVAL_STRIP_SOLID || !solid_updated) {
    time_strip_solid = millis();
    solid_updated = true;

    uint8_t r = state >> 16;
    uint8_t g = state >> 8 & 0xFF;
    uint8_t b = state & 0xFF;

    for (int i = 0; i < strip->numPixels(); i++) {
      strip->setPixelColor(i, strip->Color(r, g, b));
    }

    strip->show();
  }
}

void Strip::frame_rainbow() {
  if (millis() - time_strip_rainbow > INTERVAL_STRIP_RAINBOW) {
    time_strip_rainbow = millis();

    for (uint16_t i = 0; i < strip->numPixels(); i++) {
      strip->setPixelColor(i, strip_wheel(((((ulong)i) << 8) / strip->numPixels() + rainbow_offset) & 255));
    }

    strip->show();
    rainbow_offset++;
  }
}

void Strip::frame_stars() {
  if (millis() - time_strip_stars > INTERVAL_STRIP_STARS) {
    time_strip_stars = millis();

    for (uint16_t i = 0; i < strip->numPixels(); i++) {
			uint32_t c = strip->getPixelColor(i);

			// if pixel has color (not grayscale) then set black
			if (c % STARS_INCREMENT != state_off) {
				strip->setPixelColor(i, state_off);
			}
			// if pixel is white then set black
			else if (c >= state_on) {
				strip->setPixelColor(i, state_off);
			}
			// if pixel is grayscale then increase lightness
			else if (c > state_off && c < state_on) {
				strip->setPixelColor(i, c + STARS_INCREMENT);
			}
			// if pixel is black then start star on probability
			else {
				if (!random(STARS_PROBABILITY)) strip->setPixelColor(i, STARS_INCREMENT);
			}
    }

    strip->show();
  }
}

void Strip::frame_sunrise() {
  if (millis() - time_strip_sunrise > INTERVAL_STRIP_SUNRISE) {
    time_strip_sunrise = millis();

    for (uint16_t i = 0; i < strip->numPixels(); i++) {
      strip->setPixelColor(i, strip->Color(sunrise_offset, sunrise_offset, sunrise_offset));
    }

    if (sunrise_offset >= 255) {
      sunrise_offset = 0;
      updateState(0xffffff);
    }

    strip->show();
    sunrise_offset++;
  }
}

void Strip::frame_snake() {
	if (millis() - time_strip_snake > INTERVAL_STRIP_SNAKE) {
		time_strip_snake = millis();

		for (uint16_t i = 0; i < strip->numPixels(); i++) {
      strip->setPixelColor(i, strip->getPixelColor(i) / 2);
		}

		strip->setPixelColor(snake_food, 0xFF00FF);
		strip->setPixelColor(snake, 0x00FF00);

		if (snake_direction) {
			increase(&snake, strip->numPixels() - 1);
		}
		else {
			decrease(&snake, strip->numPixels() - 1);
		}

    if (snake == snake_food) {
      snake_food = random(strip->numPixels());
      snake_direction = (snake < snake_food) ^ (abs((int)snake - (int)snake_food) > strip->numPixels() / 2);
    }

		strip->show();
	}
}

uint32_t Strip::strip_wheel(byte angle) {
  angle = 255 - angle;
  if (angle < 85) {
    return strip->Color(255 - angle * 3, 0, angle * 3);
  }
  if (angle < 170) {
    angle -= 85;
    return strip->Color(0, angle * 3, 255 - angle * 3);
  }
  angle -= 170;
  return strip->Color(angle * 3, 255 - angle * 3, 0);
}

void Strip::increase(uint16_t* cur, uint16_t max) {
	*cur = (*cur >= max) ? 0 : *cur + 1;
}
void Strip::decrease(uint16_t* cur, uint16_t max) {
	*cur = (*cur == 0) ? max : *cur - 1;
}
#endif