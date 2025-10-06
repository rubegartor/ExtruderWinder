#pragma once

#include <WiFi.h>
#include <vector>
#include <algorithm>
#include "Commons/RPCManager.h"
#include "WiFi/secrets.h"

enum WiFiConnectionState {
  WIFI_NOT_STARTED,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_FAILED
};

struct ScannedNetwork {
  String ssid;
  int32_t rssi;
  int networkIndex;
};

class WiFiConnectionManager {
  public:
    WiFiConnectionManager(RPCManager& rpcMgr) : rpcManager(rpcMgr) {}
    
    void begin();
    void loop();
    void stop();
    
    bool isConnected() const { return connectionState == WIFI_CONNECTED; }
    WiFiConnectionState getState() const { return connectionState; }
    String getCurrentSSID() const { return currentSSID; }
    String getCurrentIP() const { return WiFi.localIP().toString(); }

  private:
    std::vector<ScannedNetwork> scanAvailableNetworks();
    bool connectToNetwork(const char* ssid, const char* password);
    void checkConnectionStatus();
    
    WiFiConnectionState connectionState = WIFI_NOT_STARTED;
    String currentSSID;
    unsigned long lastConnectionCheck = 0;
    static const unsigned long CONNECTION_CHECK_INTERVAL = 10000;
    
    RPCManager& rpcManager;
};