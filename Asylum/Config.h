// Config.h
// (ñ) hostmit
// https://github.com/hostmit/

#ifndef _CONFIG_h
#define _CONFIG_h

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <map>

#include "Debug.h"

extern "C" {
#include "spi_flash.h"
}

#define CONFIG_DEBUG

#define CONFIG_FILENAME_CONFIG "/config.json"
#define CONFIG_VALIDATOR "x"

class Config {
public:
  Config();

  std::map<String, String> current;
  std::map<String, String> predefined;

  bool loadConfig();
  void saveConfig();

  std::map<String, String> loadState(String filename);
  void saveState(String filename, std::map<String, String> states);

};

#endif