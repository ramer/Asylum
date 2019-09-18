// Encoder.cpp

#ifdef ARDUINO_ESP8266_GENERIC

#include "Encoder.h"

Encoder::Encoder(String prefix, byte action, byte eventA, byte eventB) : Device(prefix) {
  pin_eventA = eventA;
  pin_eventB = eventB;
};

void Encoder::initialize(PubSubClient* ptr_mqttClient, Config* ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  pinMode(pin_eventA, INPUT);
  pinMode(pin_eventB, INPUT);

  pinMode(pin_action, OUTPUT);

  encoderinstance = this;
  attachInterrupt(digitalPinToInterrupt(pin_eventA), EncoderInterruptFunc, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pin_eventB), EncoderInterruptFunc, CHANGE);

  generateUid();
  loadState();
}

void Encoder::update() {

  if (encoder_state != 0) {
    int new_state = constrain((int)state + encoder_state, 0, 255);
    encoder_state = 0;
    if (new_state != state) updateState(new_state);
  }

  // check state saved
  if (!state_saved && millis() - state_savedtime > INTERVAL_STATE_SAVE) { saveState(); }

  // check state published
  if (_mqttClient && _mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state); }
}

void Encoder::doEncoder() {
  bool a = digitalRead(pin_eventA);
  bool b = digitalRead(pin_eventB);

  if (pin_event_laststateA == a && pin_event_laststateB == b) return;
  pin_event_laststateA = a; pin_event_laststateB = b;

  seqA <<= 1;
  seqA |= a;

  seqB <<= 1;
  seqB |= b;

  // mask upper 4 bits
  seqA &= 0b00001111;
  seqB &= 0b00001111;

  // debug("%s %s \n", a ? "| " : " |", b ? "| " : " |");

  // for optical encoder
  //if (seqA == 0b00000011 && seqB == 0b00001001) new_state += ENCODER_STEP; // CW
  //if (seqA == 0b00001001 && seqB == 0b00000011) new_state -= ENCODER_STEP; // CCW

  // for damaged test encoder
  if (seqA == 0b00001011 && seqB == 0b00001001) encoder_state += ENCODER_STEP;
  if (seqA == 0b00001001 && seqB == 0b00000011) encoder_state -= ENCODER_STEP;
}

void Encoder::updateState(ulong state_new) {
  state_last = state_new != state_off ? state_new : state_last;
  state = state_new;

  if (state_new >= 250) { digitalWrite(pin_action, HIGH); }
  else if (state_new > 15 && state_new < 250) { analogWrite(pin_action, state_new << 2); }  // esp8266 uses 10 bit PWM
  else { digitalWrite(pin_action, LOW); }

  state_saved = false;
  state_published = false;

  state_savedtime = millis();
  state_publishedtime = millis();

  debug(" - state changed to %u \n", state_new);
}

void Encoder::invertState() {
  if (state == state_off) {
    if (state_last == 0) { state_last = 255; }
    updateState(state_last);
  }
  else {
    updateState(state_off);
  }
}

#endif