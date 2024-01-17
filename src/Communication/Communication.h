#pragma once

#include <Arduino.h>

#define SLAVE_ADDRESS 0x60

class Communication {

 public:
  void init();

  void sendTaskedEvents();

  void sendFastTaskedEvents();

  void sendEvent(String eventName, String eventData);

  void requestData();
};