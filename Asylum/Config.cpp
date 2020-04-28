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

	std::map<String, String> newconfig;

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

	const size_t capacity = 1024; //JSON_OBJECT_SIZE(predefined.size()) + 512;
	DynamicJsonDocument doc(capacity);
	DeserializationError err = deserializeJson(doc, file);

#ifdef CONFIG_DEBUG
  debug("\n-------------\tLOADING FILE\t------------\n");
	serializeJsonPretty(doc, Serial);
	debug("\n-------------\tEND OF FILE\t------------\n\n");
#endif

  if (err) {
    debug("Parsing Configuration file (%s): failed (%s). Using default configuration \n", file.name(), err.c_str());
		file.close();
		current = predefined;
    return false;
  }
  if (!doc.containsKey("validator") || doc["validator"] != CONFIG_VALIDATOR) {
    debug("Validating Configuration file (%s): failed. Using default configuration \n", file.name());
		file.close();
		current = predefined;
		return false;
	}

  for (auto &itemPredefined : predefined) {
    if (!doc.containsKey(itemPredefined.first)) {
      debug(" - configuration file does not have (%s) key, using default value (%s) \n", itemPredefined.first.c_str(), itemPredefined.second.c_str());
			newconfig.insert(itemPredefined);
    }
    else {
			newconfig.insert(std::pair<String, String>(itemPredefined.first, doc[itemPredefined.first].as<String>()));
    }
  }
  file.close();

	current = newconfig;

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
	debug("success \n");

	const size_t capacity = 1024; //JSON_OBJECT_SIZE(predefined.size());
	DynamicJsonDocument doc(capacity);

	doc["validator"] = CONFIG_VALIDATOR;

	for (auto &item : current) {
		doc[item.first] = item.second;
	}

#ifdef CONFIG_DEBUG
	debug("\n-------------\tSAVING FILE\t------------\n");
	serializeJsonPretty(doc, Serial);
	debug("\n-------------\tEND OF FILE\t------------\n\n");
#endif

	if (serializeJson(doc, file) == 0)
	{
		debug("Writing Configuration file (%s): failed. \n ", file.name());
	}
	file.close();
}

std::map<String, String> Config::loadState(String filename)
{
  std::map<String, String> states;
  File file = SPIFFS.open(filename.c_str(), "r");
  if (!file) return states;

	const size_t capacity = 128; //JSON_OBJECT_SIZE(3) + 64;
	DynamicJsonDocument doc(capacity);
	DeserializationError err = deserializeJson(doc, file);

#ifdef CONFIG_DEBUG
	debug("-------------\tLOADING FILE\t------------\n");
	serializeJsonPretty(doc, Serial);
	debug("\n-------------\tEND OF FILE\t------------\n");
#endif

  if (err) { file.close(); return states; }
  for (JsonPair jsonPair : doc.to<JsonObject>()) {
    states.insert(std::pair<String, String>(String(jsonPair.key().c_str()), jsonPair.value().as<String>()));
  }
  file.close();
  return states;
}

void Config::saveState(String filename, std::map<String, String> states)
{
  File file = SPIFFS.open(filename.c_str(), "w");
  if (!file) return;

	const size_t capacity = 128; //JSON_OBJECT_SIZE(states.size());
	DynamicJsonDocument doc(capacity);

  for (auto& statesPair : states) {
    doc[statesPair.first] = statesPair.second;
  }

#ifdef CONFIG_DEBUG
	debug("-------------\tSAVING FILE\t------------\n");
	serializeJsonPretty(doc, Serial);
  debug("\n-------------\tEND OF FILE\t------------\n");
#endif

	if (serializeJson(doc, file) == 0) {
		debug("Writing State file (%s): failed.", file.name());
	}
	file.close();
}
