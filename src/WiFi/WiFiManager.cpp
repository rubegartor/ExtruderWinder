#include "WiFi/WiFiManager.h"
#include "WiFi/secrets.h"
#include "WiFi/WebPage.h"
#include "Commons/RPCManager.h"
#include "Measurement/Measurement.h"

IPAddress staticIP(192, 168, 1, 156);     // IP estática deseada
IPAddress gateway(192, 168, 1, 1);        // Gateway (router)
IPAddress subnet(255, 255, 255, 0);       // Máscara de subred

int status = WL_IDLE_STATUS;

WiFiServer server(80);

void WiFiManager::begin() {  
  connectionState = WIFI_CONNECTING;
  WiFi.disconnect();
  WiFi.config(staticIP, gateway, subnet);

  for (int attempt = 0; attempt < 5; attempt++) {    
    status = WiFi.begin(ssid, pass, ENC_TYPE_WPA2);
    
    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
      connectionState = WIFI_CONNECTED;
      server.begin();

      rpcManager.sendWiFiInfo(WiFi.localIP().toString().c_str());
      return;
    }
  }
  
  connectionState = WIFI_FAILED;
}

void WiFiManager::stop() {
  rpcManager.sendWiFiInfo("");

  // Limpieza segura de todos los clientes
  for (auto& sseClient : sseClients) {
    if (sseClient.isValid && sseClient.client.connected()) {
      sseClient.client.stop();
    }
  }
  sseClients.clear();
  
  server.end();
  WiFi.disconnect();
  
  connectionState = WIFI_NOT_STARTED;
}

void WiFiManager::loop() {
  if (connectionState != WIFI_CONNECTED) return;

  // Limpieza periódica de clientes (cada 5 segundos)
  unsigned long now = millis();
  if (now - lastCleanup > 5000) {
    cleanupSSEClients();
    lastCleanup = now;
  }

  WiFiClient client = server.accept();
  if (client) {
    String request = "";
    
    // Timeout para evitar bloqueos
    unsigned long startTime = millis();
    const unsigned long requestTimeout = 5000; // 5 segundos de timeout

    while (client.connected() && (millis() - startTime < requestTimeout)) {
      if (client.available()) {
        request = client.readStringUntil('\n');
        break;
      }
      delay(1); // Pequeña pausa para no saturar la CPU
    }

    // Leer headers restantes de forma segura
    while (client.available() && client.readStringUntil('\n').length() > 1) {
      // Solo leer headers, no procesarlos
    }

    if (request.length() > 0) {    
      if (request.indexOf("favicon.ico") >= 0) {
        client.print("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
      } else if (request.indexOf("/events") >= 0) {
        handleSSEHandshake(client);
        return; // No cerrar el cliente aquí, se gestiona en handleSSEHandshake
      } else {
        sendWebPage(client);
      }
    }
    
    client.stop();
  }

  broadcastMeasurementSSE();
}

void WiFiManager::sendWebPage(WiFiClient& client) {  
  client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  client.print(WEB_PAGE);
}

// =====================
// SSE implementation
// =====================
void WiFiManager::handleSSEHandshake(WiFiClient& client) {
  // Verificar si podemos aceptar más clientes
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
    
    // Obtener datos de medición directamente de la instancia local
    MeasurementDataMessage measurementData = {};
    measurementData.lastRead = measurement.lastRead;
    measurementData.minRead = measurement.minRead;
    measurementData.maxRead = measurement.maxRead;
    measurementData.timestamp = measurement.lastUpdateTime;
    
    StaticJsonDocument<STATUS_JSON_CAPACITY> doc;
    buildStatusJson(doc, data, measurementData, millis());

    // Verificar que el JSON no exceda el buffer
    size_t jsonSize = measureJson(doc);
    if (jsonSize < STATUS_JSON_BUFFER) {
      client.print("data: ");
      serializeJson(doc, client);
      client.print("\n\n");
    }
  }

  // Añadir cliente de forma segura
  sseClients.emplace_back(client);
}

void WiFiManager::broadcastMeasurementSSE() {
  if (sseClients.empty()) return;
  unsigned long now = millis();
  
  if ((now - lastSseBroadcast) < SSE_PERIOD_MS) return;
  
  const SystemStatusMessage& data = rpcManager.getSystemData();
  
  // Obtener datos de medición directamente de la instancia local
  MeasurementDataMessage measurementData = {};
  measurementData.lastRead = measurement.lastRead;
  measurementData.minRead = measurement.minRead;
  measurementData.maxRead = measurement.maxRead;
  measurementData.timestamp = measurement.lastUpdateTime;
  
  lastSseBroadcast = now;

  StaticJsonDocument<STATUS_JSON_CAPACITY> doc;
  buildStatusJson(doc, data, measurementData, now);

  // Verificar tamaño del JSON antes de enviar
  size_t jsonSize = measureJson(doc);
  if (jsonSize >= STATUS_JSON_BUFFER) {
    return; // JSON demasiado grande, no enviar
  }

  char buffer[STATUS_JSON_BUFFER];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));

  // Iterar de forma segura eliminando clientes desconectados
  for (size_t i = 0; i < sseClients.size(); ) {
    auto& sseClient = sseClients[i];
    
    if (!sseClient.isValid || !isValidClient(sseClient.client)) {
      safeRemoveClient(i);
      continue;
    }
    
    sseClient.client.print("data: ");
    sseClient.client.write((const uint8_t*)buffer, len);
    sseClient.client.print("\n\n");
    
    // Verificar nuevamente la conexión después del envío
    if (!isValidClient(sseClient.client)) {
      safeRemoveClient(i);
      continue;
    }
    
    sseClient.lastActivity = now;
    ++i;
  }
}

void WiFiManager::cleanupSSEClients() {
  if (sseClients.empty()) return;
  
  unsigned long now = millis();
  
  for (size_t i = 0; i < sseClients.size(); ) {
    auto& sseClient = sseClients[i];
    
    // Verificar si el cliente es válido y está conectado
    bool shouldRemove = false;
    
    if (!sseClient.isValid) {
      shouldRemove = true;
    } else if (!isValidClient(sseClient.client)) {
      shouldRemove = true;
    } else if ((now - sseClient.lastActivity) > CLIENT_TIMEOUT_MS) {
      // Cliente inactivo por mucho tiempo
      shouldRemove = true;
    }
    
    if (shouldRemove) {
      safeRemoveClient(i);
    } else {
      ++i;
    }
  }
}

bool WiFiManager::isValidClient(WiFiClient& client) {
  return client && client.connected();
}

void WiFiManager::safeRemoveClient(size_t index) {
  if (index >= sseClients.size()) return;
  
  auto& sseClient = sseClients[index];
  if (sseClient.isValid && sseClient.client.connected()) {
    sseClient.client.stop();
  }
  sseClient.isValid = false;
  
  // Remover del vector de forma segura
  sseClients.erase(sseClients.begin() + index);
}

void WiFiManager::buildStatusJson(ArduinoJson::StaticJsonDocument<STATUS_JSON_CAPACITY>& doc, const SystemStatusMessage& data, const MeasurementDataMessage& measurement, unsigned long nowFallback) {
  JsonObject meas = doc.createNestedObject("measurement");
  meas["current"] = roundf(measurement.lastRead * 100) / 100.0f;
  meas["min"] = roundf(measurement.minRead * 100) / 100.0f;
  meas["max"] = roundf(measurement.maxRead * 100) / 100.0f;

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