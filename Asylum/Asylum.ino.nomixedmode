
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "Debug.h"
#include "Config.h"


// DEFINES

#define DEBUG
//#define DEBUG_CORE


#define WIFI_POWER                    20.5
#define PORT_DNS					            53
#define PORT_HTTP					            80

String id;
String idsuffix;

bool spiffs_ready = false;           // contains SPIFFS availability flag
bool config_ready = false;           // contains config.loadConfig() result;
bool dnsserver_ready = false;        // contains config.loadConfig() result;
bool is_updating = false;            // contains firmware uploading flag
bool got_update = false;            // contains firmware uploaded flag 
bool got_config = false;            // contains configuration updated flag 
ulong disconnectedtime = millis();  // contains time, when client was disconnected

DNSServer				dnsServer;
WiFiClient			wifiClient;
PubSubClient		mqttClient(wifiClient);
AsyncWebServer  httpServer(PORT_HTTP);
IPAddress				wifi_AP_IP(192, 168, 4, 1);
IPAddress				wifi_AP_MASK(255, 255, 255, 0);
Config          config;

WiFiEventHandler softAPModeStationConnectedHandler;
WiFiEventHandler softAPModeStationDisconnectedHandler;
WiFiEventHandler stationModeConnectedHandler;
WiFiEventHandler stationModeGotIPHandler;
WiFiEventHandler stationModeDisconnectedHandler;

void setup() {
#ifdef DEBUG
  Serial.begin(74880);
#ifdef DEBUG_CORE
  Serial.setDebugOutput(true);
#endif
#endif

  // Initialize chip
  debug("\n\n\n");
  uint8_t mac[6]; WiFi.macAddress(mac);
  for (int i = sizeof(mac) - 2; i < sizeof(mac); ++i) idsuffix += String(mac[i], HEX);
  id = "ESP-" + idsuffix;
  debug("Chip started: %s, Flash size: %u \n", id.c_str(), ESP.getFlashChipRealSize());
  if (ESP.getFlashChipSize() != ESP.getFlashChipRealSize()) debug("WARNING. Compiler flash size is different: %u \n", ESP.getFlashChipSize());
  debug("Sketch size: %u bytes, free %u bytes \n\n", ESP.getSketchSize(), ESP.getFreeSketchSpace());

  debug("Setting WiFi power to %f ... ", WIFI_POWER);
  WiFi.setOutputPower(WIFI_POWER);
  debug("done \n");

  softAPModeStationConnectedHandler =    WiFi.onSoftAPModeStationConnected(&WiFi_onSoftAPModeStationConnected);
  softAPModeStationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&WiFi_onSoftAPModeStationDisconnected);
  stationModeConnectedHandler =          WiFi.onStationModeConnected(&WiFi_onStationModeConnected);
  stationModeGotIPHandler =              WiFi.onStationModeGotIP(&WiFi_onStationModeGotIP);
  stationModeDisconnectedHandler =       WiFi.onStationModeDisconnected(&WiFi_onStationModeDisconnected);

  // Initialize config
  debug("Mounting SPIFFS ... ");
  spiffs_ready = SPIFFS.begin();
  if (spiffs_ready) {
    debug("success \n");
    config_ready = config.loadConfig();
  } else {
    debug("error \n");
    debug("WARNING. Configuration files cannot be load/save. \n");
  }
   

  //TODO Initialize devices

  //Initialize Web-interface
  debug("Starting HTTP-server ... ");
  httpServer_SetupHandlers();
  httpServer.begin();
  debug("started \n");

  if (WiFi.getMode() == WIFI_AP_STA) {
    debug("WiFi is on mixed (AP+STA) mode \n");

    debug("Scanning WiFi networks ... ");
    WiFi.scanDelete();
    int n = WiFi.scanNetworks(false, true);
    debug("done. Found %u networks \n", n);
  }

  debug("Setup done. Free heap size: %u \n", ESP.getFreeHeap());
}


void loop() {
  byte mode = config.current["mode"].toInt();
  bool APEnabled = ((WiFi.getMode() & WIFI_AP) != 0);
  bool STAEnabled = ((WiFi.getMode() & WIFI_STA) != 0);
  
  // DO NOT DO ANYTHING WHILE UPDATING
  if (is_updating) return;

  // LOOP IN AP or AP+STA MODE
  if (APEnabled && dnsserver_ready) dnsServer.processNextRequest();

  // LOOP IN CLIENT MQTT MODE
  if (STAEnabled && WiFi.isConnected() && config_ready && mode == 2 && !mqttClient.loop() && !mqttClient.connected()) mqttClient_connect();

  if (config_ready) { // Config is valid
    if (mode == 0) { // Local mode
      if (!APEnabled) StartAP();
      if (STAEnabled) StopSTA();
    }

    else if (mode == 1 || mode == 2) { // CLient or MQTT client mode
      if (!STAEnabled) StartSTA();

      if (WiFi.isConnected()) {
        if (APEnabled) StopAP();
      }
      else {
        if (millis() - disconnectedtime > 10000) { // start AP when client if disconnected for 10 secs
          if (!APEnabled) StartAP();
        }
      }
    }
  }
  else { // Config is not valid
    if (!APEnabled) StartAP();
    if (STAEnabled) StopSTA();
  }
}

void StartAP() {
  WiFi.enableAP(true);

  debug("Scanning WiFi networks ... ");
  WiFi.scanDelete();
  int n = WiFi.scanNetworks(false, true); // mode became WIFI_AP_STA or WIFI_STA
  debug("done. Found %u networks \n", n);

  debug("Starting access point ... ");
  WiFi.softAPConfig(wifi_AP_IP, wifi_AP_IP, wifi_AP_MASK);
  WiFi.softAP(id.c_str());
  debug("started (%s) \n", WiFi.softAPIP().toString().c_str());
}

void StopAP() {
  debug("Stopping access point ... ");
  WiFi.softAPdisconnect(true);
  WiFi.enableAP(false);
  debug("stopped \n");
}

void StartSTA() {
  if (config.current["apssid"].length() != 0) {
    debug("Connecting to access point: %s , password: %s \n", config.current["apssid"].c_str(), config.current["apkey"].c_str());
    WiFi.begin(config.current["apssid"].c_str(), config.current["apkey"].c_str());
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
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

void WiFi_onSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected &evt) {
  String mac; for (byte i = 0; i < sizeof(evt.mac); ++i) mac += String(evt.mac[i], HEX);
  debug("Access point client connected: %s \n", mac.c_str());

  debug("Starting DNS-server ... ");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(PORT_DNS, "*", WiFi.softAPIP());
  dnsserver_ready = true;
  debug("started \n");
}

void WiFi_onSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected &evt) {
  String mac; for (byte i = 0; i < sizeof(evt.mac); ++i) mac += String(evt.mac[i], HEX);
  debug("Access point client disconnected: %s \n", mac.c_str());

  debug("Stopping DNS-server ... ");
  dnsServer.stop();
  dnsserver_ready = false;
  debug("stopped \n");
}

void WiFi_onStationModeConnected(const WiFiEventStationModeConnected &evt) {
  debug("Connected to access point: %s \n", evt.ssid.c_str());
}

void WiFi_onStationModeGotIP(const WiFiEventStationModeGotIP &evt) {
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

  debug("Connecting to MQTT server: %s as %s , auth %s : %s ... ", config.current["mqttserver"].c_str(), id.c_str(), config.current["mqttlogin"].c_str(), config.current["mqttpassword"].c_str());

  if (mqttClient.connect(id.c_str(), config.current["mqttlogin"].c_str(), config.current["mqttpassword"].c_str())) {
    debug("connected, state = %i \n", mqttClient.state());

    //for (auto &d : devices) {
    //  d->subscribe();
    //  d->publishStatus();
    //}

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

void WiFi_onStationModeDisconnected(const WiFiEventStationModeDisconnected &evt) {
  debug("Disconnected from access point: %s, reason: %u \n", evt.ssid.c_str(), evt.reason);
  disconnectedtime = millis();
  if (config_ready && config.current["mode"].toInt() == 2) mqttClient.disconnect();
}