
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

bool spiffsready = false;     // contains SPIFFS availability flag
bool configready = false;     // contains config.loadConfig() result;
bool dnsserverready = false;  // contains config.loadConfig() result;
bool isupdating = false;      // contains firmware uploading flag
bool got_update = false;      // contains firmware uploaded flag 
bool got_config = false;      // contains configuration updated flag 
byte state = 0;               // contains current device state


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
  spiffsready = SPIFFS.begin();
  if (spiffsready) {
    debug("success \n");
    configready = config.loadConfig();
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


  if (WiFi.getMode() == WIFI_AP) {
    debug("WiFi is on access point mode \n");
    
    debug("Scanning WiFi networks ... ");
    WiFi.scanDelete();
    int n = WiFi.scanNetworks(false, true); // mode became WIFI_AP_STA
    WiFi.mode(WIFI_AP);                     // set mode back to AP
    debug("done. Found %u networks \n", n);
  }

  if (WiFi.getMode() == WIFI_STA) {
    debug("WiFi is on client mode \n");
  }

  debug("Setup done. Free heap size: %u \n", ESP.getFreeHeap());
}


void loop() {
  
  // DO NOT DO ANYTHING WHILE UPDATING
  if (isupdating) return;

  // LOOP IN AP MODE
  if (dnsserverready && WiFi.getMode() == WIFI_AP) dnsServer.processNextRequest();

  // SHOULD BE AP MODE BUT IS NOT
  if (WiFi.getMode() != WIFI_AP && (!configready || config.current["mode"].toInt() == 0)) {
    WiFi.disconnect(true);

    debug("Scanning WiFi networks ... ");
    WiFi.scanDelete();
    int n = WiFi.scanNetworks(false, true); // mode became WIFI_AP_STA or WIFI_STA
    WiFi.mode(WIFI_AP);                     // set mode back to AP
    debug("done. Found %u networks \n", n);

    debug("Starting access point ... ");
    WiFi.softAPConfig(wifi_AP_IP, wifi_AP_IP, wifi_AP_MASK);
    WiFi.softAP(id.c_str());
    debug("started (%s) \n", WiFi.softAPIP().toString().c_str());
  }

  // LOOP IN CLIENT MQTT MODE
  if (WiFi.getMode() == WIFI_STA && WiFi.isConnected() && configready && config.current["mode"].toInt() == 2 && !mqttClient.loop() && !mqttClient.connected()) mqttClient_connect();

  // SHOULD BE CLIENT MODE BUT IS NOT
  if (WiFi.getMode() != WIFI_STA && (configready && (config.current["mode"].toInt() == 1 || config.current["mode"].toInt() == 2))) {
    WiFi.disconnect(true);

    WiFi.mode(WIFI_STA);
    if (config.current["apssid"].length() != 0) {
      debug("Connecting to access point: %s , password: %s \n", config.current["apssid"].c_str(), config.current["apkey"].c_str());
      WiFi.begin(config.current["apssid"].c_str(), config.current["apkey"].c_str());
      WiFi.setAutoReconnect(true);
    }
    else {
      debug("Connecting to access point error: no SSID specified \n");
      configready = false; return;
    }
  }



}


void WiFi_onSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected &evt) {
  String mac; for (byte i = 0; i < sizeof(evt.mac); ++i) mac += String(evt.mac[i], HEX);
  debug("Access point client connected: %s \n", mac.c_str());

  debug("Starting DNS-server ... ");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(PORT_DNS, "*", WiFi.softAPIP());
  dnsserverready = true;
  debug("started \n");
}

void WiFi_onSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected &evt) {
  String mac; for (byte i = 0; i < sizeof(evt.mac); ++i) mac += String(evt.mac[i], HEX);
  debug("Access point client disconnected: %s \n", mac.c_str());

  debug("Stopping DNS-server ... ");
  dnsServer.stop();
  dnsserverready = false;
  debug("stopped \n");
}

void WiFi_onStationModeConnected(const WiFiEventStationModeConnected &evt) {
  debug("Connected to access point: %s \n", evt.ssid.c_str());
}

void WiFi_onStationModeGotIP(const WiFiEventStationModeGotIP &evt) {
  debug("Got client IP address: %s \n", evt.ip.toString().c_str());

  if (configready && config.current["mode"].toInt() == 2) {
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

  if (configready && config.current["mode"].toInt() == 2) mqttClient.disconnect();
}