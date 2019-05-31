

void mqtt_callback(char* tp, byte* pl, unsigned int length) {
  pl[length] = '\0';
  String payload = String((char*)pl);
  String topic = String(tp);

  debug(" - message recieved [%s]: %s \n", topic.c_str(), payload.c_str());

  //for (auto &d : devices) {
  //  d->handlePayload(topic, payload);

  //  if (topic == d->mqtt_topic_setup) {
  //    debug(" - setup mode command recieved \n");
  //    set_mode(2);
  //  }

  //  if (topic == d->mqtt_topic_reboot) {
  //    debug(" - reboot command recieved \n");
  //    reboot();
  //  }
  //}

}
