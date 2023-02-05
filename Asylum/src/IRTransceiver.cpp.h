// IRTransceiver.cpp.h

#ifndef _IRTransceiver_cpp_h
#define _IRTransceiver_cpp_h

#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
//#include <IRremote.h>
#include "../Device.h"

class IRTransceiver : public Device
{
public:
  IRTransceiver(String id, String prefix, byte receive, byte send);

  void initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config);

  void update();

  void publishState(String topic, IRData data);

  void handlePayload(String topic, String payload);

protected:
  ulong state_publishedinterval;
  ulong state_time;

  byte pin_receive;
  byte pin_send;

  int DELAY_BETWEEN_REPEAT = 50;

  // Storage for the recorded code
  struct storedIRDataStruct {
    IRData receivedIRData;
    // extensions for sendRaw
    uint8_t rawCode[RAW_BUFFER_LENGTH]; // The durations if raw
    uint8_t rawCodeLength; // The length of the code
  } sStoredIRData;

  void storeCode(IRData* aIRReceivedData);
  void sendCode(storedIRDataStruct* aIRDataToSend);
};

IRTransceiver::IRTransceiver(String id, String prefix, byte receive, byte send) : Device(id, prefix) {
  pin_receive = receive;
  pin_send = send;

  html_control = "";
};

void IRTransceiver::initialize(AsyncMqttClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  generateTopics();

  IrReceiver.begin(pin_receive, DISABLE_LED_FEEDBACK); // Start the receiver, enable feedback LED, take LED feedback pin from the internal boards definition
  IrSender.begin(pin_send, DISABLE_LED_FEEDBACK); // Specify send pin and enable feedback LED at default feedback LED pin
}

void IRTransceiver::update() {
  if (IrReceiver.available() && IrReceiver.decodedIRData.rawDataPtr->rawlen - 1 > 2) {
    
    storeCode(IrReceiver.read());
    publishState(mqtt_topic_pub, sStoredIRData.receivedIRData);
    
    IrReceiver.resume(); // resume receiver
  }
}

void IRTransceiver::publishState(String topic, IRData data) {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    char payload[64];
    snprintf(payload, sizeof(payload), "{\"address\":\"%u\",\"command\":\"%u\"}", data.address, data.command);
    _mqttClient->publish(topic.c_str(), 1, true, payload);

    debug(" - message sent [%s] %s \n", topic.c_str(), payload);

    state_published = true;
    state_publishedtime = millis();
  }
}

void IRTransceiver::handlePayload(String topic, String payload) {
  if (topic == mqtt_topic_sub) {
    if (payload == "repeat") {
      debug(" - repeat command received \n");

      IrReceiver.stop();
      sendCode(&sStoredIRData);
      IrReceiver.start();

      publishState(mqtt_topic_pub, sStoredIRData.receivedIRData); // force
    }
  }
}

void IRTransceiver::storeCode(IRData* aIRReceivedData) {
  if (aIRReceivedData->flags & IRDATA_FLAGS_IS_REPEAT) {
    debug(" - ignore repeat");
    return;
  }
  if (aIRReceivedData->flags & IRDATA_FLAGS_IS_AUTO_REPEAT) {
    debug(" - ignore autorepeat");
    return;
  }
  if (aIRReceivedData->flags & IRDATA_FLAGS_PARITY_FAILED) {
    debug(" - ignore parity error");
    return;
  }
  /*
   * Copy decoded data
   */
  sStoredIRData.receivedIRData = *aIRReceivedData;

//  if (sStoredIRData.receivedIRData.protocol == UNKNOWN || sStoredIRData.receivedIRData.protocol == PULSE_DISTANCE) {
    debug(" - received unknown code and store %u timing entries as raw \n", IrReceiver.decodedIRData.rawDataPtr->rawlen - 1);
    //IrReceiver.printIRResultRawFormatted(&Serial, true); // Output the results in RAW format
    IrReceiver.compensateAndPrintIRResultAsCArray(&Serial, true); // Output the results in CArray format
    sStoredIRData.rawCodeLength = IrReceiver.decodedIRData.rawDataPtr->rawlen - 1;
    IrReceiver.compensateAndStoreIRResultInArray(sStoredIRData.rawCode);
  //}
  //else {
  //  IrReceiver.printIRResultShort(&Serial);
  //  sStoredIRData.receivedIRData.flags = 0; // clear flags -esp. repeat- for later sending
  //  debug();
  //}
}

void IRTransceiver::sendCode(storedIRDataStruct* aIRDataToSend) {
  if (aIRDataToSend->receivedIRData.protocol == UNKNOWN /* i.e. raw */) {
    IrSender.sendRaw(aIRDataToSend->rawCode, aIRDataToSend->rawCodeLength, 38);
    debug(" - sent raw %u marks or spaces \n", aIRDataToSend->rawCodeLength);
  }
  else {
    IrSender.write(&aIRDataToSend->receivedIRData, NO_REPEATS);
    debug(" - sent:");
    printIRResultShort(&Serial, &aIRDataToSend->receivedIRData);
  }
}

#endif
