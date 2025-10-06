#pragma once

#include "WiFi/WiFiConnectionManager.h"
#include "WiFi/WebServerManager.h"
#include "Commons/RPCManager.h"

class WiFiManager {
  public:
    WiFiManager(RPCManager& rpcMgr) : 
        wifiConnection(rpcMgr), 
        webServer(rpcMgr) {}
    
    void begin();
    void loop();
    void stop();
    
    bool isConnected() const { return wifiConnection.isConnected(); }
    WiFiConnectionState getState() const { return wifiConnection.getState(); }

  private:
    WiFiConnectionManager wifiConnection;
    WebServerManager webServer;
};