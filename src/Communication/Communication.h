#pragma once

#include <Arduino.h>

#define SLAVE_ADDRESS 0x60
#define BUFFER_SIZE 32

class Communication {

 public:
  void init();

  void sendTaskedEvents();

  void sendFastTaskedEvents();

  void sendEvent(String eventName, String eventData);

  void requestData();
};