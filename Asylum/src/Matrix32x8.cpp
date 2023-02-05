// Matrix32x8.cpp

#include "Matrix32x8.h"

Matrix32x8::Matrix32x8(String id, String prefix, byte event, byte event2, byte event3, byte event4) : Device(id, prefix), leds(LEDS_COUNT), topo(DISPLAY_WIDTH, DISPLAY_HEIGHT) {
  html_control = "";

  pin_up = event;
  pin_down = event2;
  pin_left = event3;
  pin_right = event4;
};

void Matrix32x8::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  generateTopics();

  debug(" - initializing buttons ... ");
  btn_up.begin(pin_up, true);
  btn_down.begin(pin_down, true);
  btn_left.begin(pin_left, true);
  btn_right.begin(pin_right, true);
  debug("done \n");

  debug(" - initializing ADC ... ");
  pinMode(PIN_ADC, INPUT);
  desiredbrightness = (analogRead(PIN_ADC) >> 3);
  debug("done. value >> 3 = %u \n", desiredbrightness);

  debug(" - initializing BME280 ... ");
  Wire.begin();
  if (bme.begin()) {
    debug("done \n");
  }
  else {
    debug("failure \n");
  }

  debug(" - initializing RTC ... ");
  if (rtc.begin()) {
    if (!rtc.isrunning()) {
      debug("NOT running, adjusting upload time. \n");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    else {
      datetime = rtc.now();
      debug("done: %02u:%02u:%02u \n", datetime.hour(), datetime.minute(), datetime.second());
    }
  }
  else {
    debug("failure \n");
  }

  debug(" - initializing HTTP forecast client ... ");
  //request.onReadyStateChange(AsyncHTTPRequest_Callback, this);
  debug("done \n");

  debug(" - initializing NeoPixelBrightnessBus ... ");
  leds.Begin();
  leds.SetBrightness(desiredbrightness);
  clearframe();
  leds.Show();
  debug("done \n");
}

void Matrix32x8::update() {
  // process buttons
  if (state == INTRO) {}
  else if (state == MENU) {
    if (btn_up.update() == DOWN) {
      updateMenuSlide(menu_slide - 1);
    }
    if (btn_down.update() == DOWN) {
      updateMenuSlide(menu_slide + 1);
    }
    if (btn_right.update() == DOWN) {
      updateState(menu_slide);
    }
  }
  else if (state == PRIMARY) {
    if (btn_up.update() == PRESSED && btn_down.update() == PRESSED) {
      updateState(MENU);
    }

    if (btn_left.update() == DOWN) {
      updatePrimarySlide(primary_slide - 1);
    }
    if (btn_right.update() == DOWN) {
      updatePrimarySlide(primary_slide + 1);
    }
  }
  else if (state == TETRIS) {
  }
  else if (state == SNAKE) {
  }

  show_frame();

  if (millis() - ntp_time > INTERVAL_UPDATE_NTP) {
    ntp_time = millis();
    getntp(&rtc);
  }
}

void Matrix32x8::publishState() {
  if (!_mqttClient) return;/*
  if (_mqttClient->connected()) {
    char payload[64];
    snprintf(payload, sizeof(payload), "{\"t\":\"%.1f\",\"h\":\"%.1f\",\"p\":\"%.1f\"}", temp, hum, pres);
    _mqttClient->publish(topic.c_str(), 1, true, payload);

    debug(" - message sent [%s] %s \n", topic.c_str(), payload);

    state_published = true;
    state_publishedtime = millis();
  }*/
}

void Matrix32x8::updateState(ulong state_new) {
  state_old = state;
  state = state_new > MENU_LAST_SLIDE ? MENU_LAST_SLIDE : state_new < MENU ? MENU : state_new;

  state_published = false;

  switch (state) {
  case PRIMARY:
    updatePrimarySlide(0);
    timeline = 0;
    break;
  case MENU:
    timeline = 0;
    break;
  case TETRIS:
    tetris_reset();
    break;
  case SNAKE:
    snake_reset();
    break;
  }

  debug(" - state changed to %u \n", state);
}

void Matrix32x8::updatePrimarySlide(int8_t slide_new) {
  primary_slide = slide_new > PRIMARY_LAST_SLIDE ? 0 : slide_new < 0 ? PRIMARY_LAST_SLIDE : slide_new;
  timeline = 0;
  fadeout = false; fadein = true;
  currentbrightness = 0;
}

void Matrix32x8::updateMenuSlide(int8_t slide_new) {
  menu_slide = slide_new > MENU_LAST_SLIDE ? 1 : slide_new < 1 ? MENU_LAST_SLIDE : slide_new;
  timeline = 0;
  fadeout = false; fadein = true;
  currentbrightness = 0;
}

void Matrix32x8::show_frame() {
  if (state == INTRO) { // intro 

    if (millis() - timeline_time > INTERVAL_UPDATE_FRAME) {
      timeline_time = millis();
      prepareframe_intro();
    }

    if (millis() - brightness_time > 10) {
      brightness_time = millis();
      desiredbrightness = (desiredbrightness * (BRIGHTNESS_AVERAGE - 1) + (analogRead(PIN_ADC) >> 3)) / BRIGHTNESS_AVERAGE;
      leds.SetBrightness(desiredbrightness);
      leds.Show();
    }

  }
  else if (state == PRIMARY) {

    if (millis() - timeline_time > INTERVAL_UPDATE_FRAME) {
      timeline_time = millis();

      switch (primary_slide) {
      case 0: // time
        prepareframe_primary_time();
        break;
      case 1: // date
        prepareframe_primary_date();
        break;
      case 2: // climatetemp
        prepareframe_primary_climatetemp();
        break;
      case 3: // climatehum
        prepareframe_primary_climatehum();
        break;
      case 4: // climatepres
        prepareframe_primary_climatepres();
        break;
      case 5: // forecast
        prepareframe_primary_forecast();
        break;
      }
    }

    if (millis() - brightness_time > 10) {
      brightness_time = millis();

      desiredbrightness = (desiredbrightness * (BRIGHTNESS_AVERAGE - 1) + (analogRead(PIN_ADC) >> 3)) / BRIGHTNESS_AVERAGE;

      if (fadeout) {

        if (currentbrightness > 0) { currentbrightness--; }
        else {
          updatePrimarySlide(primary_slide + 1);
        }

      }
      else if (fadein) {

        if (currentbrightness < desiredbrightness) { currentbrightness++; }
        else { fadein = false; }

      }
      else {

        currentbrightness = desiredbrightness > 0 ? desiredbrightness : 1;

      }

      leds.SetBrightness(currentbrightness);
      leds.Show();
    }

  }
  else if (state == MENU) {

    if (millis() - timeline_time > INTERVAL_UPDATE_FRAME) {
      timeline_time = millis();

      clearframe();
      switch (menu_slide) {
      case PRIMARY:
        print_smallstring("Main", 6, RgbColor(0, 255, 255));
        break;
      case TETRIS:
        print_smallstring("Tetris", 2, RgbColor(255, 0, 255));
        break;
      case SNAKE:
        print_smallstring("Snake", 4, RgbColor(255, 255, 0));
        break;
      }
    }

    if (millis() - brightness_time > 10) {
      brightness_time = millis();

      desiredbrightness = (desiredbrightness * (BRIGHTNESS_AVERAGE - 1) + (analogRead(PIN_ADC) >> 3)) / BRIGHTNESS_AVERAGE;

      if (fadein) {

        if (currentbrightness < desiredbrightness) { currentbrightness++; }
        else { fadein = false; }

      }
      else {

        currentbrightness = desiredbrightness > 0 ? desiredbrightness : 1;

      }

      leds.SetBrightness(currentbrightness);
      leds.Show();
    }

  }
  else if (state == TETRIS) {
    if ((millis() - tetris_step_time > tetris_step_interval) ||
      (millis() - tetris_step_time > INTERVAL_UPDATE_FRAME && (btn_right.update() & (DOWN | PRESSED)))) {
      tetris_step_time = millis();
      prepareframe_tetris();
    }

    if (millis() - timeline_time > INTERVAL_UPDATE_FRAME) {
      timeline_time = millis();

      if (btn_left.update() == DOWN) tetris_rotate_figure(tetris_angle < 3 ? tetris_angle + 1 : 0);
      if (btn_down.update() & (DOWN | PRESSED)) tetris_move_figure_horizontal(tetris_horizontal + 1);
      if (btn_right.update() & (DOWN | PRESSED)) tetris_move_figure_horizontal(tetris_horizontal - 1);

      // exit to menu
      if (btn_up.update() == PRESSED && btn_down.update() == PRESSED) updateState(MENU);

      leds.Show();
    }
  }
  else if (state == SNAKE) {
    if ((millis() - snake_step_time > snake_step_interval) ||
      (millis() - snake_step_time > INTERVAL_UPDATE_FRAME && (
        (snake_direction == LEFT && (btn_left.update() == PRESSED)) ||
        (snake_direction == UP && (btn_up.update() == PRESSED)) ||
        (snake_direction == RIGHT && (btn_right.update() == PRESSED)) ||
        (snake_direction == BOTTOM && (btn_down.update() == PRESSED)))))
    {
      snake_direction = snake_nextdirection;
      snake_step_time = millis();
      prepareframe_snake();
    }

    if (millis() - timeline_time > INTERVAL_UPDATE_FRAME) {
      timeline_time = millis();

      if (btn_left.update() == DOWN && snake_direction != RIGHT) snake_nextdirection = LEFT;
      if (btn_up.update() == DOWN && snake_direction != BOTTOM) snake_nextdirection = TOP;
      if (btn_right.update() == DOWN && snake_direction != LEFT) snake_nextdirection = RIGHT;
      if (btn_down.update() == DOWN && snake_direction != TOP) snake_nextdirection = BOTTOM;

      // exit to menu
      if (btn_up.update() == PRESSED && btn_down.update() == PRESSED) updateState(MENU);

      leds.Show();
    }
  }
}

void Matrix32x8::prepareframe_intro() {
  if (timeline < 16) {
    for (uint8_t y = 0; y < 8; y++) {
      leds.SetPixelColor(topo.Map(timeline, y), Wheel((timeline << 3) & 255));
      leds.SetPixelColor(topo.Map(31 - timeline, y), Wheel(((31 - timeline) << 3) & 255));
    }
  }
  if (timeline >= 16 && timeline < 32) {
    for (uint8_t y = 0; y < 8; y++) {
      leds.SetPixelColor(topo.Map(timeline, y), Wheel(((31 - timeline) << 3) & 255));
      leds.SetPixelColor(topo.Map(31 - timeline, y), Wheel((timeline << 3) & 255));
    }
  }
  if (timeline >= 32 && timeline < 36) {
    for (uint8_t x = 0; x < 32; x++) {
      leds.SetPixelColor(topo.Map(x, 3 - (timeline & 31)), HtmlColor(0));
      leds.SetPixelColor(topo.Map(x, 4 + (timeline & 31)), HtmlColor(0));
    }
  }

  if (timeline < 32) {
    print_smallstring("esp8266", 1, 0);
  }
  else if (timeline >= 32 && timeline < 36) {
    uint8_t saturation = (timeline & 31) * 64;
    print_smallstring("esp8266", 1, RgbColor(saturation, saturation, saturation));
  }
  else {
    print_smallstring("esp8266", 1, RgbColor(255, 255, 255));
  }

  timeline++;
  if (timeline > 64) updateState(PRIMARY);
}

void Matrix32x8::prepareframe_primary_time() {
  clearframe();
    
  datetime = rtc.now();

  char t[8];
  snprintf(t, sizeof(t) + 1, "%02d:%02d:%02d", datetime.hour(), datetime.minute(), datetime.second());
  print_smallstring(t, 3, RgbColor(0, 255, 255));

  timeline++;
  if (timeline > 1000) fadeout = true;
}

void Matrix32x8::prepareframe_primary_date() {
  clearframe();

  datetime = rtc.now();

  char t[8];
  snprintf(t, sizeof(t) + 1, "%02d.%02d.%02d", datetime.day(), datetime.month(), datetime.year() - 2000);
  print_smallstring(t, 3, RgbColor(255, 150, 0));

  timeline++;
  if (timeline > 50) fadeout = true;
}

void Matrix32x8::prepareframe_primary_climatetemp() {
  clearframe();

  if (timeline == 0) { // 1st frame
    debug(" - reading bme ... ");
    bme.read(pres, temp, hum, BME280::TempUnit_Celsius, BME280::PresUnit_torr);
    debug("done \n");
  }

  char c[8];
  snprintf(c, sizeof(c) + 1, "+%i%c", (int)temp, 176);
  print_smallstring(c, 5, RgbColor(255, 255, 0));
  draw_picture(0, 24);

  timeline++;
  if (timeline > 50) fadeout = true;
}

void Matrix32x8::prepareframe_primary_climatehum() {
  clearframe();

  if (timeline == 0) { // 1st frame
    debug(" - reading bme ... ");
    bme.read(pres, temp, hum, BME280::TempUnit_Celsius, BME280::PresUnit_torr);
    debug("done \n");
  }

  char c[8];
  snprintf(c, sizeof(c) + 1, "%i%%", (int)hum);
  print_smallstring(c, 5, RgbColor(255, 255, 0));
  draw_picture(9, 24);

  timeline++;
  if (timeline > 50) fadeout = true;
}

void Matrix32x8::prepareframe_primary_climatepres() {
  clearframe();

  if (timeline == 0) { // 1st frame
    debug(" - reading bme ... ");
    bme.read(pres, temp, hum, BME280::TempUnit_Celsius, BME280::PresUnit_torr);
    debug("done \n");
  }

  char c[8];
  snprintf(c, sizeof(c) + 1, "%i", (int)pres);
  print_smallstring(c, 5, RgbColor(255, 255, 0));
  draw_picture(10, 24);

  timeline++;
  if (timeline > 50) fadeout = true;
}

void Matrix32x8::prepareframe_primary_forecast() {
  clearframe();

  char f[8];

  if (getforecast(datetime.day(), &forecast_updateday, &forecast_tempmin, &forecast_tempmax, &forecast_dayicon, &forecast_nighticon)) {

    snprintf(f, sizeof(f) + 1, "%+d%c%+d%c", (int)forecast_tempmin, 176, (int)forecast_tempmax, 176);
    print_smallstring(f, 0, RgbColor(0, 255, 0));
    if (datetime.hour() >= 6 && datetime.hour() < 23) {
      draw_picture(forecast_dayicon, 24);
    }
    else {
      draw_picture(forecast_nighticon, 24);
    }

  }
  else {

    snprintf(f, sizeof(f) + 1, "No cast");
    print_smallstring(f, 0, RgbColor(0, 255, 255));

  }

  timeline++;
  if (timeline > 50) fadeout = true;
}

void Matrix32x8::prepareframe_tetris() {
  if (!tetris_move_figure_vertical(tetris_vertical + 1)) {
    if (tetris_vertical == 0) { tetris_reset(); }
    tetris_figure = tetris_nextfigure; tetris_nextfigure = random(7);
    tetris_angle = tetris_nextangle; tetris_nextangle = random(4);
    tetris_vertical = 0; tetris_horizontal = 1;

    tetris_erase_figure(tetris_figure, tetris_angle, tetris_vertical, tetris_horizontal);
    tetris_erase_lines();
    tetris_show_figure(tetris_figure, tetris_angle, tetris_vertical, tetris_horizontal);

    if (tetris_step_interval > INTERVAL_UPDATE_FRAME) tetris_step_interval--;
  }
}

void Matrix32x8::prepareframe_snake() {
  if (!snake_move()) snake_reset();
  snake_show_map();
}

//void Matrix32x8::getforecast() {
//  static bool requestOpenResult;
//
//  if (request.readyState() == readyStateUnsent || request.readyState() == readyStateDone)
//  {
//    String url = String("http://dataservice.accuweather.com/forecasts/v1/daily/1day/") + String(FORECAST_CITY) + String("?apikey=") + String(FORECAST_APIKEY) + String("&language=ru-ru&metric=true");
//    
//    //requestOpenResult = request.open("GET", "http://worldtimeapi.org/api/timezone/Europe/London.txt");
//    requestOpenResult = request.open("GET", url.c_str());
//
//    if (requestOpenResult)
//    {
//      request.send();
//    }
//    else
//    {
//      debug(" - AsyncHTTPRequest: can't send bad request");
//    }
//  }
//  else
//  {
//    debug(" - AsyncHTTPRequest: can't send request");
//  }
//}

//void Matrix32x8::AsyncClient_Connect(void* arg, AsyncClient* client) {
//  Matrix32x8* self = (Matrix32x8*)arg;
//
//  debug(" - AsyncClient connected, Requesting data ... ");
//
//  // request data
//  if (client->canSend()) {
//    String req = String("GET /forecasts/v1/daily/1day/") + String(FORECAST_CITY) + String("?apikey=") + String(FORECAST_APIKEY) +
//      String("&language=ru-ru&metric=true HTTP/1.1\r\n") + String("Host: dataservice.accuweather.com\r\n") + String("Connection: close\r\n\r\n");
//
//    if (client->space() > req.length()) {
//      client->add(req.c_str(), req.length());
//      client->send();
//      debug("sent %u bytes \n", req.length());
//    }
//    else
//    {
//      debug("space exceeded \n");
//    }
//  }
//  else
//  {
//    debug("cannot send data \n");
//  }
//}

bool Matrix32x8::getforecast(int day, int* forecast_updateday, double* forecast_tempmin, double* forecast_tempmax, int* forecast_dayicon, int* forecast_nighticon) {
  if (WiFi.status() == WL_CONNECTED) {

    if (*forecast_updateday == day) {

      return true;

    }
    else {

      HTTPClient http;
      http.setTimeout(5000);
      http.begin(String("http://dataservice.accuweather.com/forecasts/v1/daily/1day/") + String(FORECAST_CITY) + String("?apikey=") + String(FORECAST_APIKEY) + String("&language=ru-ru&metric=true"));

      if (http.GET() > 0) {

        DynamicJsonDocument root(2048);

        debug(" - deserializing forecast Json ... ");
        // Parse JSON object
        DeserializationError error = deserializeJson(root, http.getString());
        if (error) {
          debug("failed: %s \n", error.c_str());
        }
        else
        {
          if (root.containsKey("DailyForecasts")) {
            *forecast_tempmin = root["DailyForecasts"][0]["Temperature"]["Minimum"]["Value"];
            *forecast_tempmax = root["DailyForecasts"][0]["Temperature"]["Maximum"]["Value"];
            *forecast_dayicon = root["DailyForecasts"][0]["Day"]["Icon"];
            *forecast_nighticon = root["DailyForecasts"][0]["Night"]["Icon"];

            *forecast_updateday = day;
            debug("done \n");
            http.end();
            return true;
          }
          else
          {
            debug("failed: Json not contains DailyForecasts key \n");
          }
        }
      }
      http.end();
      return false;
    }
  }
  else {
    return (*forecast_updateday == day);
  }
}


//void Matrix32x8::AsyncHTTPRequest_Callback(void* optParm, AsyncHTTPRequest* request, int readyState) {
//  Matrix32x8* self = (Matrix32x8*)optParm;
//  
//  debug(" - AsyncClient data received(state = %u) \n", readyState);
//
//  if (readyState == readyStateDone) {
//    String response = request->responseText();
//
//    debug(" - AsyncClient data response: \n%s\n", response.c_str());
//
//    DynamicJsonDocument root(2048);
//
//    debug(" - deserializing Json ... ");
//    // Parse JSON object
//    DeserializationError error = deserializeJson(root, response);
//    if (error) {
//      debug("failed: %s \n", error.c_str());
//    }
//    else
//    {
//      if (root.containsKey("DailyForecasts")) {
//        self->forecast_tempmin = root["DailyForecasts"][0]["Temperature"]["Minimum"]["Value"];
//        self->forecast_tempmax = root["DailyForecasts"][0]["Temperature"]["Maximum"]["Value"];
//        self->forecast_dayicon = root["DailyForecasts"][0]["Day"]["Icon"];
//        self->forecast_nighticon = root["DailyForecasts"][0]["Night"]["Icon"];
//        self->forecast_day = self->rtc.now().day();
//        debug("done \n");
//      }
//      else
//      {
//        debug("failed: Json not contains DailyForecasts key \n");
//      }
//    }
//  }
//}

bool Matrix32x8::getntp(RTC_DS1307* dt) {
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
    debug(" - initializing NTP ... ");
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
    dt->adjust(DateTime(now() + 2)); // now is current time from time library, updated by NTP. 2 secs added to compensate RTC adjust time
    debug(" - NTP sync: %s \n", NTP.getTimeDateString().c_str());
  }
}

bool Matrix32x8::print_string(String data, uint16_t xoffset, RgbColor color) {
  bool pixelaffected = false;

  for (uint8_t c = 0; c < data.length(); c++) {
    uint8_t glyphlen = 0;

    // calculating glyph length to ignore tail spaces
    for (uint8_t glyphcolumn = 0; glyphcolumn < 8; glyphcolumn++) {
      if (pgm_read_byte(&Glyph[(data[c] << 3) + 7 - glyphcolumn]) != 0x0) { glyphlen = 7 - glyphcolumn + 1; break; }
    }

    // glyph length for space or undefined char
    if (glyphlen == 0) { xoffset += 2; continue; }

    for (uint8_t glyphcolumn = 0; glyphcolumn < glyphlen; glyphcolumn++) {
      if (xoffset >= 0 && xoffset < 32) {
        byte glyphcolumndata = pgm_read_byte(&Glyph[(data[c] << 3) + glyphcolumn]);
        for (uint8_t y = 0; y < 8; y++) {
          if (bitRead(glyphcolumndata, y) == 1) {
            leds.SetPixelColor(topo.Map(xoffset, y), color);
            pixelaffected = true;
          }
        }
      }
      xoffset++; // next glyph column
    }

    xoffset++; // next char
  }

  return pixelaffected;
}

bool Matrix32x8::print_smallstring(String data, uint16_t xoffset, RgbColor color) {
  //uint8_t len = data.length();
  //char* buf = new char[len];
  //utf8_to_cp1251(data.c_str(), buf, len);
  //data = utf8rus(data);
  bool pixelaffected = false;

  for (uint8_t c = 0; c < data.length(); c++) {
    uint8_t glyphlen = 0;

    // calculating glyph length to ignore tail spaces
    for (uint8_t glyphcolumn = 0; glyphcolumn < 8; glyphcolumn++) {
      if (pgm_read_byte(&SmallGlyph[(data[c] << 3) + 7 - glyphcolumn]) != 0x0) { glyphlen = 7 - glyphcolumn + 1; break; }
    }

    // glyph length for space or undefined char
    if (glyphlen == 0) { xoffset += 2; continue; }

    for (uint8_t glyphcolumn = 0; glyphcolumn < glyphlen; glyphcolumn++) {
      if (xoffset >= 0 && xoffset < 32) {
        byte glyphcolumndata = pgm_read_byte(&SmallGlyph[(data[c] << 3) + glyphcolumn]);
        for (uint8_t y = 0; y < 8; y++) {
          if (bitRead(glyphcolumndata, y) == 1) {
            leds.SetPixelColor(topo.Map(xoffset, y), color);
            pixelaffected = true;
          }
        }
      }
      xoffset++; // next glyph column
    }

    xoffset++; // next char
  }

  return pixelaffected;
}

void Matrix32x8::print_updating() {
  leds.ClearTo(0);
  print_smallstring("update", 0, RgbColor(0, 255, 0));
  leds.Show();
}

void Matrix32x8::draw_picture(uint16_t index, byte xoffset) {
  for (uint8_t x = 0; x < 8; x++) {
    for (uint8_t y = 0; y < 8; y++) {
      if (xoffset + x >= 0 && xoffset + x < DISPLAY_WIDTH) leds.SetPixelColor(topo.Map(xoffset + x, y), HtmlColor(pgm_read_dword(&Color[pgm_read_byte(&WeatherPictures[(index << 6) + (x << 3) + y])])));
    }
  }
}

void Matrix32x8::clearframe() {
  //for (uint16_t i = 0; i < leds.PixelCount(); i++) {
  //  leds.SetPixelColor(i, 0);
  //}
  leds.ClearTo(0);
}

RgbColor Matrix32x8::Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return RgbColor(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return RgbColor(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return RgbColor(WheelPos * 3, 255 - WheelPos * 3, 0);
}

String Matrix32x8::utf8rus(String source)
{
  int i, k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length(); i = 0;

  while (i < k) {
    n = source[i]; i++;

    if (n >= 0xC0) {
      switch (n) {
      case 0xD0: {
        n = source[i]; i++;
        if (n == 0x81) { n = 0xA8; break; }
        if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
        break;
      }
      case 0xD1: {
        n = source[i]; i++;
        if (n == 0x91) { n = 0xB8; break; }
        if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
        break;
      }
      }
    }
    m[0] = n; target = target + String(m);
  }
  return target;
}


#pragma region tetris

void Matrix32x8::tetris_reset()
{
  tetris_step_interval = INTERVAL_TETRIS_STARTSPEED;
  tetris_step_time = millis();
  tetris_nextfigure = 0, tetris_figure = 0, tetris_nextangle = 0, tetris_angle = 0;
  tetris_vertical = 0, tetris_horizontal = 1;
  leds.ClearTo(0);
  tetris_step_interval = INTERVAL_TETRIS_STARTSPEED;
}

bool Matrix32x8::tetris_check_collision(uint16_t f, uint16_t a, int8_t v, int8_t h) {
  for (byte x = 0; x < 4; x++)
    for (byte y = 0; y < 4; y++)
      if (pgm_read_dword(&Color[pgm_read_byte(&Figures[(f << 6) + (a << 4) + (x << 2) + y])]) != 0x0) // inspecting pixel is not empty
        if ((y + h >= DISPLAY_HEIGHT) || (y + h < 0)) {
          Serial.printf("collision! not enought height: figure:%u angle:%u v:%u h:%u \n", f, a, v, h);
          return true;
        }
        else if (x + v >= DISPLAY_WIDTH) {
          Serial.printf("collision! not enought width: figure:%u angle:%u v:%u h:%u \n", f, a, v, h);
          return true;
        }
        else if (leds.GetPixelColor(topo.Map(v + x, h + y)) != 0) {
          Serial.printf("collision! pixel is busy: figure:%u angle:%u v:%u h:%u \n", f, a, v, h);
          return true;
        }
  return false;
}

void Matrix32x8::tetris_show_figure(uint16_t f, uint16_t a, int8_t v, int8_t h) {
  for (uint8_t x = 0; x < 4; x++) {
    for (uint8_t y = 0; y < 4; y++) {
      uint32_t color = pgm_read_dword(&Color[pgm_read_byte(&Figures[(f << 6) + (a << 4) + (x << 2) + y])]);
      if (color != 0) leds.SetPixelColor(topo.Map(v + x, h + y), HtmlColor(color));
    }
  }
}

void Matrix32x8::tetris_erase_figure(uint16_t f, uint16_t a, int8_t v, int8_t h) {
  for (uint8_t x = 0; x < 4; x++) {
    for (uint8_t y = 0; y < 4; y++) {
      uint32_t color = pgm_read_dword(&Color[pgm_read_byte(&Figures[(f << 6) + (a << 4) + (x << 2) + y])]);
      if (color != 0) leds.SetPixelColor(topo.Map(v + x, h + y), 0);
    }
  }
}

void Matrix32x8::tetris_erase_lines() {
  for (uint8_t x = 0; x < DISPLAY_WIDTH; x++) {
    bool fullline = true;
    for (uint8_t y = 0; y < DISPLAY_HEIGHT; y++) {
      if (leds.GetPixelColor(topo.Map(x, y)) == 0) {
        fullline = false;
      }
    }
    if (fullline) {
      for (uint8_t v = 0; v < x - 1; v++) {
        for (uint8_t h = 0; h < DISPLAY_HEIGHT; h++) {
          leds.SetPixelColor(topo.Map(x - v, h), leds.GetPixelColor(topo.Map(x - v - 1, h)));
        }
      }
    }
  }
}

bool Matrix32x8::tetris_move_figure_vertical(int8_t new_vertical) {
  tetris_erase_figure(tetris_figure, tetris_angle, tetris_vertical, tetris_horizontal);
  bool success = !tetris_check_collision(tetris_figure, tetris_angle, new_vertical, tetris_horizontal);
  if (success) tetris_vertical = new_vertical;
  tetris_show_figure(tetris_figure, tetris_angle, tetris_vertical, tetris_horizontal);
  if (tetris_vertical > 4) tetris_show_figure(tetris_nextfigure, tetris_nextangle, 0, 1);
  return success;
}

void Matrix32x8::tetris_move_figure_horizontal(int8_t new_horizontal) {
  tetris_erase_figure(tetris_figure, tetris_angle, tetris_vertical, tetris_horizontal);
  if (!tetris_check_collision(tetris_figure, tetris_angle, tetris_vertical, new_horizontal)) tetris_horizontal = new_horizontal;
  tetris_show_figure(tetris_figure, tetris_angle, tetris_vertical, tetris_horizontal);
}

void Matrix32x8::tetris_rotate_figure(uint16_t new_angle) {
  tetris_erase_figure(tetris_figure, tetris_angle, tetris_vertical, tetris_horizontal);
  if (!tetris_check_collision(tetris_figure, new_angle, tetris_vertical, tetris_horizontal)) tetris_angle = new_angle;
  tetris_show_figure(tetris_figure, tetris_angle, tetris_vertical, tetris_horizontal);
}

#pragma endregion

#pragma region snake

void Matrix32x8::snake_reset()
{
  leds.ClearTo(0);

  for (byte x = 0; x < DISPLAY_WIDTH; x++)
    for (byte y = 0; y < DISPLAY_HEIGHT; y++)
      snake_map[x][y] = 0;

  snake_step_interval = INTERVAL_SNAKE_STARTSPEED;
  snake_lenght = INTERVAL_SNAKE_STARTLENGHT;
  snake_crazy = 0;
  snake_head.y = random(DISPLAY_HEIGHT); snake_head.x = random(DISPLAY_WIDTH);

  snake_direction = random(2) == 0 ? LEFT : RIGHT;
  for (byte l = 0; l < snake_lenght; l++)
    snake_map[snake_direction == RIGHT ? snake_head.x - l : snake_head.x + l][snake_head.y] = snake_lenght - l;

  snake_give_food();
}

void Matrix32x8::snake_show_map()
{
  clearframe();
  for (byte x = 0; x < DISPLAY_WIDTH; x++)
    for (byte y = 0; y < DISPLAY_HEIGHT; y++) {
      if (snake_map[x][y] > 0) leds.SetPixelColor(topo.Map(x, y), RgbColor(0, (byte)(245 * snake_map[x][y] / snake_lenght) + 10, 0));
      if (snake_map[x][y] == -1) leds.SetPixelColor(topo.Map(x, y), RgbColor(255, 0, 0));
      if (snake_map[x][y] == -2) leds.SetPixelColor(topo.Map(x, y), RgbColor(0, 0, 255));
    }
  leds.SetPixelColor(topo.Map(snake_head.x, snake_head.y), snake_crazy > 0 ? RgbColor(0, 255, 255) : RgbColor(255, 255, 0));
}

void Matrix32x8::snake_give_food()
{
  point food;
  do {
    food.x = random(DISPLAY_WIDTH);
    food.y = random(DISPLAY_HEIGHT);
  } while (snake_map[food.x][food.y] > 0);
  snake_map[food.x][food.y] = -1;
}

void Matrix32x8::snake_give_extrafood()
{
  if (random(50) > 0) return;

  for (byte x = 0; x < DISPLAY_WIDTH; x++)
    for (byte y = 0; y < DISPLAY_HEIGHT; y++)
      if (snake_map[x][y] == -2) snake_map[x][y] = 0;

  point extrafood;
  do {
    extrafood.x = random(DISPLAY_WIDTH);
    extrafood.y = random(DISPLAY_HEIGHT);
  } while (snake_map[extrafood.x][extrafood.y] > 0);
  snake_map[extrafood.x][extrafood.y] = -2;
}

bool Matrix32x8::snake_move() {
  snake_head.x = snake_direction == LEFT ? snake_head.x - 1 : snake_direction == RIGHT ? snake_head.x + 1 : snake_head.x;
  snake_head.y = snake_direction == TOP ? snake_head.y - 1 : snake_direction == BOTTOM ? snake_head.y + 1 : snake_head.y;
  if (snake_head.x < 0) snake_head.x = DISPLAY_WIDTH - 1; if (snake_head.x > DISPLAY_WIDTH - 1)  snake_head.x = 0;
  if (snake_head.y < 0) snake_head.y = DISPLAY_HEIGHT - 1; if (snake_head.y > DISPLAY_HEIGHT - 1) snake_head.y = 0;

  if (snake_map[snake_head.x][snake_head.y] == -1) { // take food
    if (snake_step_interval > INTERVAL_UPDATE_FRAME) snake_step_interval--; // increase speed
    snake_lenght++;
    snake_map[snake_head.x][snake_head.y] = snake_lenght; // move head
    snake_give_food();
    snake_give_extrafood();
    return true;
  }
  if (snake_map[snake_head.x][snake_head.y] == -2) { // take extrafood
    if (snake_step_interval > INTERVAL_UPDATE_FRAME + 10) snake_step_interval -= 10; // increase speed
    snake_lenght++;
    snake_map[snake_head.x][snake_head.y] = snake_lenght; // move head
    snake_crazy = (snake_lenght >> 1) + 5;
    return true;
  }
  if (snake_map[snake_head.x][snake_head.y] > 0) { // bite self
    if (snake_crazy > 0) {
      snake_crazy = 0;
      byte taillen = snake_map[snake_head.x][snake_head.y];
      if (taillen < (snake_lenght >> 1)) { // if tail less then a half of snake
        snake_map[snake_head.x][snake_head.y] = snake_lenght; // update bite place
        snake_lenght -= taillen;
        for (byte x = 0; x < DISPLAY_WIDTH; x++)
          for (byte y = 0; y < DISPLAY_HEIGHT; y++)
            if (snake_map[x][y] > 0)
              snake_map[x][y] = snake_map[x][y] >= taillen ? snake_map[x][y] - taillen : 0;
        return true;
      }
    }
    return false;
  }
  else { // regular move
    for (byte x = 0; x < DISPLAY_WIDTH; x++)
      for (byte y = 0; y < DISPLAY_HEIGHT; y++)
        if (snake_map[x][y] > 0) snake_map[x][y]--;

    if (snake_crazy > 0) snake_crazy--;
    snake_map[snake_head.x][snake_head.y] = snake_lenght; // move head
    return true;
  }
}

#pragma endregion
