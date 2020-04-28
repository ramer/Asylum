// Device.h

#ifndef _DEVICE_h
#define _DEVICE_h

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "Config.h"
#include "Button.h"

#define INTERVAL_STATE_PUBLISH		    200
#define INTERVAL_STATE_SAVE		        1000

class Device
{
public:
  Device(String prefix, byte event, byte action);
  Device(String prefix);
  virtual ~Device();

  virtual void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);

  virtual void update();
  virtual void updateState(ulong state_new);
  virtual void invertState();
  virtual void handlePayload(String topic, String payload);

  virtual void subscribe();
  virtual void publishState(String topic, ulong statepayload);

  //void onUpdateState(std::function<void(ulong)> onUpdateStateCallback);

  ulong state = 0;
  ulong state_last = 0;
  ulong state_on = 1;
  ulong state_off = 0;

  String uid;
  String uid_prefix;
  String uid_macsuffix;
  String uid_filename;
  String mqtt_topic_pub;
  String mqtt_topic_sub;

  String html_control = R"~(
    <div class="field-group">
        <input id="uid" type="checkbox" onclick="onCheckboxClick(event)" class="toggle">
            <label for="uid">OFF</label>
        </input>
    </div>)~";

protected:
  virtual void generateUid();
  virtual void loadState();
  virtual void saveState();

  //std::function<void(ulong)> updateStateCallback;

  AsyncMqttClient* _mqttClient;
  Config* _config;

  bool  state_published = false;
  ulong	state_publishedtime = 0;

  bool  state_saved = false;
  ulong	state_savedtime = 0;

  byte  pin_event;
  byte  pin_action;

  Button btn;
};

#endif