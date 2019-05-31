
extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

void httpServer_SetupHandlers() {
  //httpServer.serveStatic("/", SPIFFS, "/www/"); // .setDefaultFile("setup.html"); // .setAuthentication(config.cur_conf["locallogin"].c_str(), config.cur_conf["localpassword"].c_str());

  httpServer.on("/setup", HTTP_GET, handleSetup);
  httpServer.on("/style.css", HTTP_GET, handleStyle);
  httpServer.on("/submit", HTTP_POST, handleSubmit);
  httpServer.on("/api_config", HTTP_GET, handleApiConfig);
  httpServer.on("/upload", HTTP_GET, handleUpload);
  httpServer.on("/update", HTTP_POST, handleUpdate, handleFileUpload);
  httpServer.on("/fs", HTTP_GET, handleFS);

  httpServer.onNotFound(handleRedirect);
}

void handleSetup(AsyncWebServerRequest *request) {
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  request->send_P(200, "text/html", setup_html);
}

void handleStyle(AsyncWebServerRequest *request) {
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  request->send_P(200, "text/css", style_html);
}

void handleSubmit(AsyncWebServerRequest *request) {
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  for (auto &itemDefault : config.predefined) {
    if (!request->hasParam(itemDefault.first, true)) {
      debug("API configuration does not have (%s) key, use default value (%s) \n", itemDefault.first.c_str(), itemDefault.second.c_str());
      config.current[itemDefault.first] = itemDefault.second;
    }
    else {
      config.current[itemDefault.first] = request->getParam(itemDefault.first, true)->value();
    }
  }

  config.saveConfig();

  request->send(200, "text/html", "Configuration saved.");
  got_config = true;
}

void handleApiConfig(AsyncWebServerRequest *request) {
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["uid"] = id;
  for (auto &item : config.current) {
    root[item.first] = item.second;
  }

  JsonArray& networks = root.createNestedArray("networks");

  int n = WiFi.scanComplete();
  int t = 0;
  if (n > 0) {
    int8_t lastmax = 0;

    for (int j = 0; j < n; j++) {
      int8_t max = -100;

      for (int i = 0; i < n; i++) {
        if (WiFi.RSSI(i) > max && WiFi.RSSI(i) < lastmax) { max = WiFi.RSSI(i); };
      }

      lastmax = max;

      for (int i = 0; i < n; i++) {
        if (WiFi.RSSI(i) == max) {
          JsonObject& network = networks.createNestedObject();
          network["ssid"] = WiFi.SSID(i);
          network["name"] = (WiFi.encryptionType(i) == ENC_TYPE_NONE ? "🔓 " : "🔒 ") + get_quality(WiFi.RSSI(i)) + " &emsp; " + WiFi.SSID(i) + " &emsp; " + (WiFi.isHidden(i) ? "👁" : " ");
          t++; if (t >= 10) goto print;
        }
      }
    }
  }

print:

  root.printTo(*response);
  request->send(response);

  debug("HTTP-Server: config requested \n");
}

void handleUpload(AsyncWebServerRequest *request) {
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  request->send_P(200, "text/html", upload_html);
}

void handleUpdate(AsyncWebServerRequest *request) {
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  bool update_spiffs = false;

  if (request->hasParam("spiffs", true)) {
    update_spiffs = (request->getParam("spiffs", true)->value() == "1");
  }

  got_update = !update_spiffs && !Update.hasError();
  request->send_P(200, "text/html", Update.hasError() ? updatefailure_html : updatesuccess_html);
}

void handleFileUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  bool update_spiffs = false;

  if (request->hasParam("spiffs", true)) {
    update_spiffs = (request->getParam("spiffs", true)->value() == "1");
  }

  if (index == 0) {
    debug("Upload started: %s \n", filename.c_str());
    Update.runAsync(true);
    isupdating = true;

    if (update_spiffs) {
      size_t spiffsSize = ((size_t)&_SPIFFS_end - (size_t)&_SPIFFS_start);
      if (Update.begin(spiffsSize, U_SPIFFS)) {
        debug("Updating SPIFFS ... ");
      }
      else {
        debug("Update SPIFFS begin failure: ");
        Update.printError(Serial);
      }
    }
    else {
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (Update.begin(maxSketchSpace)) {
        debug("Updating firmware ... ");
      }
      else {
        debug("Update firmware begin failure: ");
        Update.printError(Serial);
      }
    }
  }

  if (!Update.hasError()) {
    if (Update.write(data, len) != len) {
      debug("write error: ");
      Update.printError(Serial);
      debug(" \n");
    }
  }
  else {
    isupdating = false;
  }

  if (final) {
    if (Update.end(true)) {
      debug("done. \n");
    }
    else {
      debug("Update end failure: ");
      Update.printError(Serial);
    }
    isupdating = false;
  }
}

void handleFS(AsyncWebServerRequest *request) {
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();

  Dir dir = SPIFFS.openDir("/");
  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[") {
      output += ',';
    }
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  request->send(200, "text/json", output);
}

void handleRedirect(AsyncWebServerRequest *request) {
  debug("HTTP-Server: request redirected from: %s \n", request->url().c_str());
  AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
  response->addHeader("Location", String("http://") + (WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + String("/setup"));
  request->send(response);
}

String get_quality(int32_t rssi) {
  switch ((rssi + 110) / 10)
  {
  case 5:
    return "▇";
    break;
  case 4:
    return "▆";
    break;
  case 3:
    return "▅";
    break;
  case 2:
    return "▃";
    break;
  default:
    return "▁";
    break;
  }
}
