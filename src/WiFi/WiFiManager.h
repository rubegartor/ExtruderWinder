#pragma once

#include <WiFi.h>
#include <vector>
#include <ArduinoJson.h>
#include "Commons/RPCManager.h"

// Forward declaration
class Measurement;

#define SSE_PERIOD_MS 200
#define MAX_SSE_CLIENTS 5  // Límite máximo de clientes SSE
#define CLIENT_TIMEOUT_MS 30000  // Timeout para clientes inactivos

enum WiFiConnectionState {
  WIFI_NOT_STARTED,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_FAILED
};

class WiFiManager {
  public:
    WiFiManager(RPCManager& rpcMgr, Measurement& measurementRef) : rpcManager(rpcMgr), measurement(measurementRef) {}
    
    void begin();
    void loop();
    void stop();

  private:
    void sendWebPage(WiFiClient& client);
    void handleSSEHandshake(WiFiClient& client);
    void broadcastMeasurementSSE();
    void cleanupSSEClients();
    bool isValidClient(WiFiClient& client);
    void safeRemoveClient(size_t index);

    static constexpr size_t STATUS_JSON_CAPACITY = 1024;  // Aumentado para seguridad
    static constexpr size_t STATUS_JSON_BUFFER   = 896;   // Aumentado proporcionalmente

    void buildStatusJson(ArduinoJson::StaticJsonDocument<STATUS_JSON_CAPACITY>& doc, const SystemStatusMessage& data, const MeasurementDataMessage& measurement, unsigned long nowFallback);

    WiFiServer server;
    
    // Estructura para gestión segura de clientes
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
    
    WiFiConnectionState connectionState = WIFI_NOT_STARTED;
    RPCManager& rpcManager;
    Measurement& measurement;
};