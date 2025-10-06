#include "WiFi/WiFiManager.h"

void WiFiManager::begin() {
  wifiConnection.begin();
  
  if (wifiConnection.isConnected()) {
    webServer.begin();
  }
}

void WiFiManager::loop() {
  wifiConnection.loop();
  
  if (wifiConnection.isConnected()) {
    webServer.loop();
  }
}

void WiFiManager::stop() {
  webServer.stop();
  wifiConnection.stop();
}
