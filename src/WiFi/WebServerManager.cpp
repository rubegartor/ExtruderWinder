#include "WiFi/WebServerManager.h"
#include "WiFi/WebPage.h"

WiFiServer server(80);

void WebServerManager::begin() {
  server.begin();
  serverRunning = true;
}

void WebServerManager::stop() {
  for (auto& sseClient : sseClients) {
    if (sseClient.isValid && sseClient.client.connected()) {
      sseClient.client.stop();
    }
  }
  sseClients.clear();
  
  server.end();
  serverRunning = false;
}

void WebServerManager::loop() {
  if (!serverRunning) return;

  unsigned long now = millis();
  if (now - lastCleanup > 5000) {
    cleanupSSEClients();
    lastCleanup = now;
  }

  WiFiClient client = server.accept();
  if (client) {
    String request = "";
    
    unsigned long startTime = millis();
    const unsigned long requestTimeout = 5000;

    while (client.connected() && (millis() - startTime < requestTimeout)) {
      if (client.available()) {
        request = client.readStringUntil('\n');
        break;
      }
      delay(1);
    }

    while (client.available() && client.readStringUntil('\n').length() > 1) {
    }

    if (request.length() > 0) {    
      if (request.indexOf("favicon.ico") >= 0) {
        client.print("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
      } else if (request.indexOf("/events") >= 0) {
        handleSSEHandshake(client);
        return;
      } else {
        sendWebPage(client);
      }
    }
    
    client.stop();
  }

  broadcastMeasurementSSE();
}

void WebServerManager::sendWebPage(WiFiClient& client) {  
  client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  client.print(WEB_PAGE);
}

void WebServerManager::handleSSEHandshake(WiFiClient& client) {
  if (sseClients.size() >= MAX_SSE_CLIENTS) {
    client.print("HTTP/1.1 503 Service Unavailable\r\nConnection: close\r\n\r\n");
    client.stop();
    return;
  }

  client.print(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/event-stream\r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: keep-alive\r\n"
    "Access-Control-Allow-Origin: *\r\n\r\n");

  client.print(":ok\n\n");

  if (rpcManager.hasNewSystemData()) {
    const SystemStatusMessage& data = rpcManager.getSystemData();
    
    StaticJsonDocument<STATUS_JSON_CAPACITY> doc;
    buildStatusJson(doc, data, millis());

    size_t jsonSize = measureJson(doc);
    if (jsonSize < STATUS_JSON_BUFFER) {
      client.print("data: ");
      serializeJson(doc, client);
      client.print("\n\n");
    }
  }

  sseClients.emplace_back(client);
}

void WebServerManager::broadcastMeasurementSSE() {
  if (sseClients.empty()) return;
  unsigned long now = millis();
  
  if ((now - lastSseBroadcast) < SSE_PERIOD_MS) return;
  
  const SystemStatusMessage& data = rpcManager.getSystemData();
  
  lastSseBroadcast = now;

  StaticJsonDocument<STATUS_JSON_CAPACITY> doc;
  buildStatusJson(doc, data, now);

  size_t jsonSize = measureJson(doc);
  if (jsonSize >= STATUS_JSON_BUFFER) {
    return;
  }

  char buffer[STATUS_JSON_BUFFER];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));

  for (size_t i = 0; i < sseClients.size(); ) {
    auto& sseClient = sseClients[i];
    
    if (!sseClient.isValid || !isValidClient(sseClient.client)) {
      safeRemoveClient(i);
      continue;
    }
    
    sseClient.client.print("data: ");
    sseClient.client.write((const uint8_t*)buffer, len);
    sseClient.client.print("\n\n");
    
    if (!isValidClient(sseClient.client)) {
      safeRemoveClient(i);
      continue;
    }
    
    sseClient.lastActivity = now;
    ++i;
  }
}

void WebServerManager::cleanupSSEClients() {
  if (sseClients.empty()) return;
  
  unsigned long now = millis();
  
  for (size_t i = 0; i < sseClients.size(); ) {
    auto& sseClient = sseClients[i];
    
    bool shouldRemove = false;
    
    if (!sseClient.isValid) {
      shouldRemove = true;
    } else if (!isValidClient(sseClient.client)) {
      shouldRemove = true;
    } else if ((now - sseClient.lastActivity) > CLIENT_TIMEOUT_MS) {
      shouldRemove = true;
    }
    
    if (shouldRemove) {
      safeRemoveClient(i);
    } else {
      ++i;
    }
  }
}

bool WebServerManager::isValidClient(WiFiClient& client) {
  return client && client.connected();
}

void WebServerManager::safeRemoveClient(size_t index) {
  if (index >= sseClients.size()) return;
  
  auto& sseClient = sseClients[index];
  if (sseClient.isValid && sseClient.client.connected()) {
    sseClient.client.stop();
  }
  sseClient.isValid = false;
  
  sseClients.erase(sseClients.begin() + index);
}

void WebServerManager::buildStatusJson(ArduinoJson::StaticJsonDocument<STATUS_JSON_CAPACITY>& doc, const SystemStatusMessage& data, unsigned long nowFallback) {
  JsonObject meas = doc.createNestedObject("measurement");
  meas["current"] = roundf(data.measurementLastRead * 100) / 100.0f;
  meas["min"] = roundf(data.measurementMinRead * 100) / 100.0f;
  meas["max"] = roundf(data.measurementMaxRead * 100) / 100.0f;

  JsonObject conf = doc.createNestedObject("configuration");
  float diameter = data.currentDiameter;
  if (isnan(diameter) || isinf(diameter) || diameter <= 0) diameter = 0.00f;
  conf["diameter"] = roundf(diameter * 100) / 100.0f;
  conf["plasticType"] = data.plasticTypeIndex;

  JsonObject motors = doc.createNestedObject("motors");
  JsonObject puller = motors.createNestedObject("puller");
  puller["speed"] = data.pullerSpeed;
  puller["revolutions"] = data.pullerRevolutions;
  puller["minOutput"] = data.pullerMinOutput;
  puller["maxOutput"] = data.pullerMaxOutput;

  JsonObject spooler = motors.createNestedObject("spooler");
  spooler["speed"] = data.spoolerSpeed;
  spooler["revolutions"] = data.spoolerRevolutions;

  JsonObject aligner = motors.createNestedObject("aligner");
  aligner["position"] = data.alignerPosition;
  aligner["state"] = data.alignerState;
  aligner["direction"] = data.alignerDirection;
  aligner["calibrated"] = data.isSpoolCalibrated;
  if (data.isSpoolCalibrated) {
    aligner["startPos"] = data.alignerStartPos;
    aligner["endPos"] = data.alignerEndPos;
    aligner["startExtension"] = data.alignerStartExtension;
    aligner["endExtension"] = data.alignerEndExtension;
  }
  aligner["maxPosition"] = data.alignerMaxPosition;

  JsonObject tensioner = doc.createNestedObject("tensioner");
  tensioner["distance"] = data.tensionerDistance;

  JsonObject production = doc.createNestedObject("production");
  production["extrudedLength"] = roundf(data.extrudedLength * 100) / 100.0f;
  production["extrudedWeight"] = roundf(data.extrudedWeight * 10) / 10.0f;

  JsonObject system = doc.createNestedObject("system");
  system["calibrated"] = data.isSpoolCalibrated;
  unsigned long timestamp = data.timestamp; if (timestamp == 0) timestamp = nowFallback;
  system["timestamp"] = timestamp;
}