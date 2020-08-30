
extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

void httpServer_SetupHandlers() {
  httpServer.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html").setAuthentication(config.current["locallogin"].c_str(), config.current["localpassword"].c_str());
  httpServer.serveStatic("/script.js", SPIFFS, "/www/script.js");
  httpServer.serveStatic("/style.css", SPIFFS, "/www/style.css");
  httpServer.serveStatic("/favicon.ico", SPIFFS, "/www/favicon.ico");

  //httpServer.on("/setup", HTTP_GET, handleSetup);
  //httpServer.on("/script.js", HTTP_GET, handleScript);
  //httpServer.on("/style.css", HTTP_GET, handleStyle);
  //httpServer.on("/favicon.ico", HTTP_GET, handleFavicon);

  httpServer.on("/upload", HTTP_GET, handleUpload);
  httpServer.on("/submit", HTTP_POST, handleSubmit);
  httpServer.on("/api_config", HTTP_GET, handleApiConfig);
  httpServer.on("/api_controls", HTTP_GET, handleApiControls);
  httpServer.on("/api_command", HTTP_POST, handleApiCommand);
  httpServer.on("/update", HTTP_POST, handleUpdate, handleFileUpload);
  httpServer.on("/reboot", HTTP_GET, handleReboot, handleFileUpload);
  httpServer.on("/fs", HTTP_GET, handleFS);

  httpServer.onNotFound(handleRedirect);
}

//void handleSetup(AsyncWebServerRequest *request) {
//  debug(" - handleSetup \n");
//  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
//  request->send_P(200, "text/html", setup_html);
//}

//void handleScript(AsyncWebServerRequest* request) {
//  debug(" - handleScript \n");
//  request->send_P(200, "text/javascript", script_js);
//}

//void handleStyle(AsyncWebServerRequest *request) {
//  debug(" - handleStyle \n");
//  request->send_P(200, "text/css", style_html);
//}

//void handleFavicon(AsyncWebServerRequest* request) {
//  //debug(" - handleFavicon \n");
//  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
//  request->send_P(200, "image/x-icon", favicon_ico, sizeof(favicon_ico));
//}

void handleSubmit(AsyncWebServerRequest *request) {
  //debug(" - handleSubmit \n");
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  for (auto &itemPredefined : config.predefined) {
    if (!request->hasParam(itemPredefined.first, true)) {
      debug(" - API configuration does not have (%s) key, use default value (%s) \n", itemPredefined.first.c_str(), itemPredefined.second.c_str());
      config.current[itemPredefined.first] = itemPredefined.second;
    }
    else {
      config.current[itemPredefined.first] = request->getParam(itemPredefined.first, true)->value();
    }
  }

  config.saveConfig();
  config_updated = true;

  request->send(200, "text/html", "Configuration saved.");
}

void handleApiConfig(AsyncWebServerRequest *request) {
  debug(" - handleApiConfig \n");
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  AsyncResponseStream *response = request->beginResponseStream("application/json");

	const size_t capacity = 2048; //JSON_OBJECT_SIZE(config.current.size());
	DynamicJsonDocument doc(capacity);

	doc["uid"] = id;
  for (auto &item : config.current) {
		doc[item.first] = item.second;
  }

  JsonArray networks = doc.createNestedArray("networks");

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
        if (WiFi.RSSI(i) == max && WiFi.isHidden(i) == false) {
          JsonObject network = networks.createNestedObject();
          network["ssid"] = WiFi.SSID(i);
          network["name"] = (WiFi.encryptionType(i) == ENC_TYPE_NONE ? "🔓 " : "🔒 ") + get_quality(WiFi.RSSI(i)) + "&emsp;" + WiFi.SSID(i);
          t++; if (t >= 10) goto print;
        }
      }
    }
  }

print:

	serializeJson(doc, *response);
  request->send(response);
}

void handleApiControls(AsyncWebServerRequest* request) {
  debug(" - handleApiControls \n");
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();

  String controls;

  for (auto& d : devices) {
    controls = controls + d->html_control;
  }
  request->send(200, "text/html", controls);
}

void handleApiCommand(AsyncWebServerRequest* request) {
  debug(" - handleApiCommand \n");
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();

  if (request->hasParam("state", true) && request->hasParam("id", true)) {
    ulong newstate = request->getParam("state", true)->value().toInt();

    for (auto& d : devices) {
      if (d->uid == request->getParam("id", true)->value()) {
        d->updateState(newstate);
        request->send(200, "text/html", String(d->state));
      }
    }
  }
  else {
    debug("HTTP-Server: received to less parameters \n");
  }

  request->send(200, "text/html", "0");
}

void handleUpload(AsyncWebServerRequest *request) {
  debug(" - handleUpload \n");
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  request->send_P(200, "text/html", upload_html);
}

void handleReboot(AsyncWebServerRequest* request) {
  //debug(" - handleReboot \n");
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  debug(" - reboot command recieved \n");
  need_reboot = true;
  request->send_P(200, "text/html", "Rebooting...");
}

void handleUpdate(AsyncWebServerRequest *request) {
  if (!(!config.current["locallogin"].length() || !config.current["localpassword"].length() || request->authenticate(config.current["locallogin"].c_str(), config.current["localpassword"].c_str()))) return request->requestAuthentication();
  bool update_spiffs = false;

  if (request->hasParam("spiffs", true)) {
    update_spiffs = (request->getParam("spiffs", true)->value() == "1");
  }

  if (update_spiffs && !Update.hasError()) config.saveConfig();

  need_reboot = !update_spiffs && !Update.hasError();
  request->send_P(200, "text/html", Update.hasError() ? "Update failed." : "Update completed successfully.");
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
    is_updating = true;

    if (update_spiffs) {
      size_t spiffsSize = ((size_t)&_SPIFFS_end - (size_t)&_SPIFFS_start);
      if (Update.begin(spiffsSize, U_FS)) {
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
    is_updating = false;
  }

  if (final) {
    if (Update.end(true)) {
      debug("done. \n");
    }
    else {
      debug("Update end failure: ");
      Update.printError(Serial);
    }
    is_updating = false;
  }
}

void handleFS(AsyncWebServerRequest *request) {
  //debug(" - handleFS \n");
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
  //debug(" - handleRedirect \n");
  bool sta_ip = (request->host() == WiFi.localIP().toString());
  debug("HTTP-Server: request redirected from: %s \n", request->url().c_str());
  AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
  response->addHeader("Location", String("http://") + (sta_ip ? WiFi.localIP().toString() : WiFi.softAPIP().toString()));
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
