#pragma once

#include <WiFi.h>
#include <vector>
#include <ArduinoJson.h>
#include "Commons/RPCManager.h"

#define SSE_PERIOD_MS 200
#define MAX_SSE_CLIENTS 5
#define CLIENT_TIMEOUT_MS 30000

class WebServerManager {
  public:
    WebServerManager(RPCManager& rpcMgr) : rpcManager(rpcMgr) {}
    
    void begin();
    void loop();
    void stop();
    
    bool isRunning() const { return serverRunning; }

  private:
    void sendWebPage(WiFiClient& client);
    void handleSSEHandshake(WiFiClient& client);
    void broadcastMeasurementSSE();
    void cleanupSSEClients();
    bool isValidClient(WiFiClient& client);
    void safeRemoveClient(size_t index);
    
    static constexpr size_t STATUS_JSON_CAPACITY = 1024;
    static constexpr size_t STATUS_JSON_BUFFER = 896;
    
    void buildStatusJson(ArduinoJson::StaticJsonDocument<STATUS_JSON_CAPACITY>& doc, const SystemStatusMessage& data, unsigned long nowFallback);

    WiFiServer server;
    bool serverRunning = false;
    
    struct SSEClient {
      WiFiClient client;
      unsigned long lastActivity;
      bool isValid;
      
      SSEClient() : lastActivity(0), isValid(false) {}
      SSEClient(WiFiClient c) : client(c), lastActivity(millis()), isValid(true) {}
    };
    
    std::vector<SSEClient> sseClients;
    unsigned long lastSseBroadcast = 0;
    unsigned long lastCleanup = 0;
    
    RPCManager& rpcManager;
};