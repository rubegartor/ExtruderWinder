#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include "AsyncTCP.h"

#define WIFI_HOSTNAME "filamentExtruder"

#define WIFI_MAX_RETRIES 10

class WifiOut {
 private:
  uint8_t retries;

  void startServer();

 public:
  bool connected;

  String ipAddr;

  void connect();

  void handleEvents();

  void putEvent(const char* eventName, String eventData);
};