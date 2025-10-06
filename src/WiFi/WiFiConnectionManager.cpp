#include "WiFi/WiFiConnectionManager.h"

IPAddress staticIP(192, 168, 1, 156);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

int status = WL_IDLE_STATUS;

void WiFiConnectionManager::begin() {
  connectionState = WIFI_CONNECTING;
  WiFi.disconnect();
  WiFi.config(staticIP, gateway, subnet);

  std::vector<ScannedNetwork> availableNetworks = scanAvailableNetworks();
  
  if (availableNetworks.empty()) {
    connectionState = WIFI_FAILED;
    return;
  }
  
  std::sort(availableNetworks.begin(), availableNetworks.end(), 
    [](const ScannedNetwork& a, const ScannedNetwork& b) {
      return a.rssi > b.rssi;
    });

  for (const auto& network : availableNetworks) {
    const WiFiCredentials& credentials = wifiNetworks[network.networkIndex];
    
    if (connectToNetwork(credentials.ssid, credentials.password)) {
      connectionState = WIFI_CONNECTED;
      currentSSID = credentials.ssid;
      rpcManager.sendWiFiInfo(WiFi.localIP().toString().c_str(), credentials.ssid);
      return;
    }
  }
  
  connectionState = WIFI_FAILED;
}

void WiFiConnectionManager::stop() {
  rpcManager.sendWiFiInfo("", "");
  WiFi.disconnect();
  connectionState = WIFI_NOT_STARTED;
  currentSSID = "";
}

void WiFiConnectionManager::loop() {
  unsigned long now = millis();
  if (now - lastConnectionCheck > CONNECTION_CHECK_INTERVAL) {
    checkConnectionStatus();
    lastConnectionCheck = now;
  }
}

void WiFiConnectionManager::checkConnectionStatus() {
  if (connectionState == WIFI_CONNECTED && WiFi.status() != WL_CONNECTED) {
    connectionState = WIFI_FAILED;
    currentSSID = "";
    rpcManager.sendWiFiInfo("", "");
  }
}

std::vector<ScannedNetwork> WiFiConnectionManager::scanAvailableNetworks() {
  std::vector<ScannedNetwork> availableNetworks;
  int numNetworks = WiFi.scanNetworks();
  
  if (numNetworks == 0) {
    return availableNetworks;
  }
  
  for (int i = 0; i < numNetworks; i++) {
    String scannedSSID = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);

    for (int j = 0; j < totalNetworks; j++) {
      if (scannedSSID == wifiNetworks[j].ssid) {
        ScannedNetwork network;
        network.ssid = scannedSSID;
        network.rssi = rssi;
        network.networkIndex = j;
        availableNetworks.push_back(network);
        break;
      }
    }
  }
  
  return availableNetworks;
}

bool WiFiConnectionManager::connectToNetwork(const char* ssid, const char* password) {
  status = WiFi.begin(ssid, password, ENC_TYPE_WPA2);
  
  for (int i = 0; i < 3 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
  }
  
  return WiFi.status() == WL_CONNECTED;
}