// Config.cpp

#include "Config.h"

Config::Config() {
  predefined = {
    { "description", "" },
    { "mode", "0" },
    { "apssid", "" },
    { "apkey", "" },
    { "locallogin", "" },
    { "localpassword", "" },
    { "mqttserver", "" },
    { "mqttlogin", "" },
    { "mqttpassword", "" },
    { "onboot", "0" },
    { "onboardled", "0" },
    { "extension1", "" },
    { "extension2", "" },
    { "extension3", "" }
  };
}

bool Config::loadConfig() {
  debug("Reading Configuration file (%s) ... ", String(CONFIG_FILENAME_CONFIG).c_str());

  File file = SPIFFS.open(CONFIG_FILENAME_CONFIG, "r");
  if (!file)
  {
    debug("failed. Using default configuration \n");
    current = predefined;
    return false;
  }
  else {
    debug("success \n");
  }

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(file);

  //debug("\n-------------\tLOADING FILE\t------------\n");
  //root.prettyPrintTo(Serial);
  //debug("\n-------------\tEND OF FILE\t------------\n\n");

  if (!root.success()) {
    debug("Parsing Configuration file (%s): failed. Using default configuration \n", file.name());
    current = predefined;
    file.close();
    return false;
  }
  if (!root.containsKey("validator") || root["validator"] != CONFIG_VALIDATOR) {
    debug("Validating Configuration file (%s): failed. Using default configuration \n", file.name());
    current = predefined;
    file.close();
    return false;
  }

  for (auto &itemPredefined : predefined) {
    if (!root.containsKey(itemPredefined.first)) {
      debug("Configuration file (%s) does not have (%s) key, using default value (%s)", file.name(), itemPredefined.first.c_str(), itemPredefined.second.c_str());
      current.insert(itemPredefined);
    }
    else {
      current[itemPredefined.first] = root[itemPredefined.first].as<String>();
    }
  }
  file.close();

  return true;
}

void Config::saveConfig() {
  debug("Saving Configuration file (%s) ... ", String(CONFIG_FILENAME_CONFIG).c_str());

  File file = SPIFFS.open(CONFIG_FILENAME_CONFIG, "w");
  if (!file)
  {
    debug("failed: cannot be created \n");
    return;
  }
  else {
    debug("success \n");
  }

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  root["validator"] = CONFIG_VALIDATOR;

  for (auto &item : current) {
    root[item.first] = item.second;
  }

  //debug("\n-------------\tSAVING FILE\t------------\n");
  //root.prettyPrintTo(Serial);
  //debug("\n-------------\tEND OF FILE\t------------\n\n");

  if (root.printTo(file) == 0)
  {
    debug("Writing Configuration file (%s): failed.", file.name());
  }

  file.close();
}

std::map<String, String> Config::loadState(String filename)
{
  std::map<String, String> states;
  File file = SPIFFS.open(filename.c_str(), "r");
  if (!file) return states;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(file);

  //debug("-------------\tLOADING FILE\t------------\n");
  //root.prettyPrintTo(Serial);
  //debug("\n-------------\tEND OF FILE\t------------\n");

  if (!root.success()) { file.close(); return states; }
  for (auto& jsonPair : root) {
    states.insert(std::pair<String, String>(String(jsonPair.key), String(jsonPair.value.asString())));
  }
  file.close();
  return states;
}

void Config::saveState(String filename, std::map<String, String> states)
{
  File file = SPIFFS.open(filename.c_str(), "w");
  if (!file) return;

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  for (auto& statesPair : states) {
    root[statesPair.first] = statesPair.second;
  }

  //debug("-------------\tSAVING FILE\t------------\n");
  //root.prettyPrintTo(Serial);
  //debug("\n-------------\tEND OF FILE\t------------\n");

  root.printTo(file);
  file.close();
}
