
#include "Button.h"
#include "Device.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Hash.h>

#include "Debug.h"
#include "Config.h"
#include "Device.h"

#include "src/Strip.h"
#include "src/Encoder.h"
#include "src/Socket.h"


// GLOBAL FIRMWARE CONFIGURATION

#define DEVICE_TYPE_SOCKET
//#define DEVICE_TYPE_TOUCHT1

//#define DEVICE_TYPE_MOTOR
//#define DEVICE_TYPE_STRIP
//#define DEVICE_TYPE_ENCODER
//#define DEVICE_TYPE_DISPLAY_32x8

//#define DEVICE_TYPE_ANALOGSENSOR


// DEFINES

//#define DEBUG_CORE

#define WIFI_POWER              20.5
#define PORT_DNS                53
#define PORT_HTTP               80
#define INTERVAL_STATUS_LED     500
#define INTERVAL_STA_RECONNECT  30000

String id;
String id_macsuffix;
String mqtt_global_topic_status;
String mqtt_global_topic_setup;
String mqtt_global_topic_reboot;

bool spiffs_ready = false;      // contains SPIFFS availability flag
bool config_ready = false;      // contains config.loadConfig() result;
bool dnsserver_ready = false;   // contains config.loadConfig() result;
bool is_updating = false;       // contains firmware uploading flag
bool need_reboot = false;       // contains reboot flag
bool config_updated = false;    // contains config updated flag
bool force_ap = false;          // contains forced AP mode flag
ulong force_ap_time;            // contains forced AP mode time
ulong last_reconnect_time;      // contains last disconnected time in STA mode
byte connect_attempts = 0;      // contains connection attempts, increases when client was disconnected

DNSServer       dnsServer;
WiFiClient      wifiClient;
PubSubClient    mqttClient(wifiClient);
AsyncWebServer  httpServer(PORT_HTTP);

IPAddress       wifi_AP_IP(192, 168, 4, 1);
IPAddress       wifi_AP_MASK(255, 255, 255, 0);
Config          config;

std::vector<Device*> devices;

WiFiEventHandler softAPModeStationConnectedHandler;
WiFiEventHandler softAPModeStationDisconnectedHandler;
WiFiEventHandler stationModeConnectedHandler;
WiFiEventHandler stationModeGotIPHandler;
WiFiEventHandler stationModeDisconnectedHandler;

ulong time_status_led;
bool  state_status_led;

void setup() {
	//delay(5000);
  Serial.begin(74880);
#ifdef DEBUG_CORE
  Serial.setDebugOutput(true);
#endif

#if (defined DEVICE_TYPE_SOCKET)
#define STATUS_LED 13                                  //inverted
	devices.push_back(new Socket("Socket1", 0, 12));     // event, action
#endif
#if (defined DEVICE_TYPE_TOUCHT1)
#define STATUS_LED 13                                  //inverted
  devices.push_back(new Socket("TouchT1-1", 0, 12));   // event, action
  devices.push_back(new Socket("TouchT1-2", 9, 5));    // event, action
  //devices.push_back(new Socket("TouchT1-3", 10, 4)); // event, action
#endif

// IMPORTANT: use Generic ESP8266 Module
#if (defined DEVICE_TYPE_MOTOR                          && defined ARDUINO_ESP8266_GENERIC)    
  devices.push_back(new Motor("Motor", 0, 12, 14));     // event, direction, action
#endif
#if (defined DEVICE_TYPE_STRIP                          && defined ARDUINO_ESP8266_GENERIC)
	//Adafruit_NeoPixel strip = ;
	devices.push_back(new Strip("Strip", 13));            // action
#endif
#if (defined DEVICE_TYPE_ENCODER                        && defined ARDUINO_ESP8266_GENERIC)
  devices.push_back(new Encoder("Encoder", 14, 12, 13));// action, A, B
#endif
#if (defined DEVICE_TYPE_DISPLAY_32x8                   && defined ARDUINO_ESP8266_GENERIC)
  //#include "devices/Display_32x8.h"
  //devices.push_back(new Display_32x8("Display_32x8", 0, 12, 2, 13));        // up, down, left, right
#endif

// IMPORTANT: use Amperka WiFi Slot
#if (defined DEVICE_TYPE_ANALOGSENSOR                   && defined ARDUINO_AMPERKA_WIFI_SLOT)
#define PIN_MODE	A5	                                  // inverted
#define PIN_LED   A2                                    // inverted
  devices.push_back(new AnalogSensor("AnalogSensor", A5, A2, A6));      // event, action, sensor
#endif 

  // Initialize chip
  debug("\n\n\n");
  uint8_t mac[6]; WiFi.macAddress(mac);
  for (int i = sizeof(mac) - 2; i < sizeof(mac); ++i) id_macsuffix += String(mac[i], HEX);
  
  id = devices.size() == 1 ? devices[0]->uid_prefix + "-" + id_macsuffix : "ESP-" + id_macsuffix;
  mqtt_global_topic_status = id + "/status";
  mqtt_global_topic_setup = id + "/setup";
  mqtt_global_topic_reboot = id + "/reboot";

  debug("Chip started: %s, Flash size: %u \n", id.c_str(), ESP.getFlashChipRealSize());
  if (ESP.getFlashChipSize() != ESP.getFlashChipRealSize()) debug("WARNING. Compiler flash size is different: %u \n", ESP.getFlashChipSize());
  debug("Sketch size: %u bytes, free %u bytes \n\n", ESP.getSketchSize(), ESP.getFreeSketchSpace());

  debug("Setting WiFi power to %f ... ", WIFI_POWER);
  WiFi.setOutputPower(WIFI_POWER);
  debug("done \n");

  softAPModeStationConnectedHandler = WiFi.onSoftAPModeStationConnected(&WiFi_onSoftAPModeStationConnected);
  softAPModeStationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&WiFi_onSoftAPModeStationDisconnected);
  stationModeConnectedHandler = WiFi.onStationModeConnected(&WiFi_onStationModeConnected);
  stationModeGotIPHandler = WiFi.onStationModeGotIP(&WiFi_onStationModeGotIP);
  stationModeDisconnectedHandler = WiFi.onStationModeDisconnected(&WiFi_onStationModeDisconnected);

  // Initialize config
  debug("Mounting SPIFFS ... ");
  spiffs_ready = SPIFFS.begin();
  if (spiffs_ready) {
		debug("success \n");

		FSInfo fsinfo;
		SPIFFS.info(fsinfo);
		debug(" - totalBytes: %s \n", String(fsinfo.totalBytes).c_str());
		debug(" - usedBytes: %s \n", String(fsinfo.usedBytes).c_str());
		debug(" - blockSize: %s \n", String(fsinfo.blockSize).c_str());
		debug(" - pageSize: %s \n", String(fsinfo.pageSize).c_str());
		debug(" - maxOpenFiles: %s \n", String(fsinfo.maxOpenFiles).c_str());
		debug(" - maxPathLength: %s \n", String(fsinfo.maxPathLength).c_str());

    config_ready = config.loadConfig();
    if (config_ready) debug("Initial config mode is %u \n", config.current["mode"].toInt());
  }
  else {
    debug("error \n");
    debug("WARNING. Configuration files cannot be load/save. \n");
  }
    
  // Initialize devices
  debug("Initializing devices \n");
  for (auto& d : devices) {
    d->initialize(&mqttClient, &config);
    debug("Initialized: %s \n", d->uid.c_str());
  }

  //Initialize Web-interface
  debug("Starting HTTP-server ... ");
  httpServer_SetupHandlers();
  httpServer.begin();
  debug("started \n");

  byte mode = WiFi.getMode();
  if ((mode & WIFI_AP) != 0) {
    StartAP();
  }
  if (((mode & WIFI_STA) != 0)) {
    StartSTA();
  }
  
  mode = WiFi.getMode(); // mode could be changed
  debug("Current WiFi mode is %s \n", mode == 1 ? "STA" : mode == 2 ? "AP" : mode == 3 ? "AP_STA" : "OFF");

  debug("Setup done. Free heap size: %u \n\n", ESP.getFreeHeap());
}


void loop() {
  byte configmode = config.current["mode"].toInt();
  byte mode = WiFi.getMode();
  bool APEnabled = ((mode & WIFI_AP) != 0);
  bool STAEnabled = ((mode & WIFI_STA) != 0);

  // DO NOT DO ANYTHING WHILE UPDATING
  if (is_updating) return;

  // LOOP IN AP or AP+STA MODE
  if (APEnabled && dnsserver_ready) dnsServer.processNextRequest();

  // LOOP IN CLIENT MQTT MODE
  if (STAEnabled && WiFi.isConnected() && config_ready && configmode == 2 && !mqttClient.loop() && !mqttClient.connected()) mqttClient_connect();

  if (config_ready) { // Config is valid
    if (configmode == 0) { // Local mode
      if (STAEnabled) StopSTA();
      if (!APEnabled) StartAP();
    }
    else if (configmode == 1 || configmode == 2) { // Client or MQTT client mode
      if (WiFi.isConnected()) {
        if (APEnabled && WiFi.softAPgetStationNum() == 0 && (!force_ap || (millis() - force_ap_time > 60000))) {
          StopAP();
          if (force_ap) force_ap = false;
        }
      }
      else {
        if (millis() - last_reconnect_time > INTERVAL_STA_RECONNECT) {
          last_reconnect_time = millis();
          WiFi.reconnect();
        }
        if (connect_attempts >= 3) { // start AP when client if disconnected for 30 secs
          if (!APEnabled) StartAP();
        }
      }

      if (!STAEnabled) StartSTA();
    }
  }
  else { // Config is not valid
    if (STAEnabled) StopSTA();
    if (!APEnabled) StartAP();
  }


  // DO ANYWAY

  for (auto& d : devices) {
    d->update();
  }

  if (config_updated) {
    config_updated = false;
		config_ready = true;
    debug("Config updated. \n");
    if (configmode == 0) { StopAP(); }
    if (configmode == 1 || configmode == 2) { StopSTA(); }
  }

  if (need_reboot) {
    need_reboot = false;
    debug("\n\nChip restarting.\n\n");
    ESP.restart();
  }

#ifdef STATUS_LED
  blynk(!config_ready || ((configmode == 1 || configmode == 2) && APEnabled));
#endif
}

void StartAP() {
  WiFi.enableAP(true);

  ScanNetworks();

  debug("Starting access point ... ");
  WiFi.softAPConfig(wifi_AP_IP, wifi_AP_IP, wifi_AP_MASK);
  WiFi.softAP(id.c_str());
  debug("started (%s) \n", WiFi.softAPIP().toString().c_str());
}

void StartAPForce() {
  force_ap = true;
  force_ap_time = millis();
  StartAP();
}

void StopAP() {
  debug("Stopping access point ... ");
  WiFi.softAPdisconnect(true);
  WiFi.enableAP(false);
  debug("stopped \n");
}

void StartSTA() {
  if (config_ready && config.current["apssid"].length() != 0) {
    debug("Connecting to access point: %s , password: %s \n", config.current["apssid"].c_str(), config.current["apkey"].c_str());
    WiFi.begin(config.current["apssid"].c_str(), config.current["apkey"].c_str());
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(false);
  }
  else {
    debug("Connecting to access point error: no SSID specified \n");
    config_ready = false; return;
  }
}

void StopSTA() {
  debug("Stopping client ... ");
  WiFi.disconnect(true);
  WiFi.enableSTA(false);
  debug("stopped \n");
}

void ScanNetworks() {
  // Do scan when
  //     WiFi is not client mode        OR   if client    is   connected  or    client     is    idle
  if (((WiFi.getMode() & WIFI_STA) == 0) || (WiFi.status() == WL_CONNECTED || WiFi.status() == WL_IDLE_STATUS)) { // in other case STA will be disconnected
    WiFiMode_t lastmode = WiFi.getMode();   // remember last mode
    debug("Scanning WiFi networks ... ");
    int n = WiFi.scanNetworks(false, true); // mode became WIFI_AP_STA or WIFI_STA
    debug("done. Found %u networks \n", n);
    WiFi.mode(lastmode);                    // set last mode back
  }
}

void WiFi_onSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
  String mac; for (byte i = 0; i < sizeof(evt.mac); ++i) mac += String(evt.mac[i], HEX);
  debug("Access point client connected: %s \n", mac.c_str());

  debug("Starting DNS-server ... ");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(PORT_DNS, "*", WiFi.softAPIP());
  dnsserver_ready = true;
  debug("started \n");

  force_ap = false;  // force AP mode was enabled by MQTT or button for 1 minute
}


void WiFi_onSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt) {
  String mac; for (byte i = 0; i < sizeof(evt.mac); ++i) mac += String(evt.mac[i], HEX);
  debug("Access point client disconnected: %s \n", mac.c_str());

  debug("Stopping DNS-server ... ");
  dnsServer.stop();
  dnsserver_ready = false;
  debug("stopped \n");
}

void WiFi_onStationModeConnected(const WiFiEventStationModeConnected& evt) {
  debug("Connected to access point: %s \n", evt.ssid.c_str());
  connect_attempts = 0;
}

void WiFi_onStationModeGotIP(const WiFiEventStationModeGotIP& evt) {
  debug("Got client IP address: %s \n", evt.ip.toString().c_str());

  if (config_ready && config.current["mode"].toInt() == 2) {
    debug("Configuring MQTT-client ... ");
    mqttClient.setServer(config.current["mqttserver"].c_str(), 1883);
    mqttClient.setCallback(mqtt_callback);
    debug("configured \n");
  }
}

void mqttClient_connect() {
  if (mqttClient.connected()) return;
  
  uint8_t mac_int[6];
  WiFi.macAddress(mac_int);
  String mac_str = "";
  for (int i = 0; i < sizeof(mac_int); ++i) {
    mac_str += String(mac_int[i], HEX);
  }

  char willmessage[MQTT_MAX_PACKET_SIZE];
  snprintf(willmessage, sizeof(willmessage), "{\"mac\":\"%s\",\"status\":\"disconnected\",\"desc\":\"%s\"}", mac_str.c_str(), config.current["description"].c_str());

  debug("Connecting to MQTT server with Will message: %s as %s , auth %s : %s ... ", config.current["mqttserver"].c_str(), id.c_str(), config.current["mqttlogin"].c_str(), config.current["mqttpassword"].c_str());
	bool connected = mqttClient.connect(id.c_str(), config.current["mqttlogin"].c_str(), config.current["mqttpassword"].c_str(), mqtt_global_topic_status.c_str(), 0, true, willmessage, true);
	if (!connected) {
		debug("failed, state = %i \n", mqttClient.state());
		debug("Connecting to MQTT server: %s as %s , auth %s : %s ... ", config.current["mqttserver"].c_str(), id.c_str(), config.current["mqttlogin"].c_str(), config.current["mqttpassword"].c_str());
		connected = mqttClient.connect(id.c_str(), config.current["mqttlogin"].c_str(), config.current["mqttpassword"].c_str());
	}
	if (connected) {
    debug("connected, state = %i \n", mqttClient.state());

    mqttClient.subscribe(mqtt_global_topic_setup.c_str());
    mqttClient.subscribe(mqtt_global_topic_reboot.c_str());

    for (auto& d : devices) {
      d->subscribe();
    }

    char payload[MQTT_MAX_PACKET_SIZE];
    snprintf(payload, sizeof(payload), "{\"mac\":\"%s\",\"ip\":\"%s\",\"status\":\"connected\",\"desc\":\"%s\"}", mac_str.c_str(), WiFi.localIP().toString().c_str(), config.current["description"].c_str());
    mqttClient.publish(mqtt_global_topic_status.c_str(), payload, true);

    debug(" - message sent [%s] %s \n", mqtt_global_topic_status.c_str(), payload);
  }
  else
  {
    debug("failed, state = %i \n", mqttClient.state());
  }

  // -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
  // -3 : MQTT_CONNECTION_LOST - the network connection was broken
  // -2 : MQTT_CONNECT_FAILED - the network connection failed
  // -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
  //  0 : MQTT_CONNECTED - the client is connected
  //  1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
  //  2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
  //  3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
  //  4 : MQTT_CONNECT_BAD_CREDENTIALS - the username / password were rejected
  //  5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
}

void WiFi_onStationModeDisconnected(const WiFiEventStationModeDisconnected& evt) {
  debug("Disconnected from access point: %s, reason: %u, connect attempt: %u \n", evt.ssid.c_str(), evt.reason, connect_attempts);
  connect_attempts += 1;
  last_reconnect_time = millis();
  if (config_ready && config.current["mode"].toInt() == 2) mqttClient.disconnect();
}

void mqtt_callback(char* tp, byte* pl, unsigned int length) {
  pl[length] = '\0';
  String payload = String((char*)pl);
  String topic = String(tp);

  debug(" - message recieved [%s]: %s \n", topic.c_str(), payload.c_str());
  
  if (topic == mqtt_global_topic_setup) {
    debug(" - setup mode command recieved \n");
    StartAPForce();
  }

  if (topic == mqtt_global_topic_reboot) {
    debug(" - reboot command recieved \n");
    need_reboot = true;
  }

  for (auto &d : devices) {
    d->handlePayload(topic, payload);
  }
}

void blynk(bool setup) {

#ifdef STATUS_LED
  pinMode(STATUS_LED, OUTPUT);

  if (setup)
  {
    if (millis() - time_status_led > INTERVAL_STATUS_LED) {
      time_status_led = millis();
      state_status_led = !state_status_led;
      digitalWrite(STATUS_LED, !state_status_led); // LED circuit inverted
    }
  }
  else {
    digitalWrite(STATUS_LED, (config.current["onboardled"].toInt() == 0 ? HIGH : LOW)); // LED circuit inverted
  }

#endif
}