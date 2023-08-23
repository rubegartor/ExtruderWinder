#include <Commons/Commons.h>
#include <FS.h>
#include <WiFi.h>
#include <WiFi/WifiCredentials.h>
#include <WiFi/WifiOut.h>
#include <WiFiMulti.h>

#include "SPIFFS.h"

WiFiMulti wifiMulti;
AsyncWebServer server(80);
AsyncEventSource events("/events");

void WifiOut::connect() {
  delay(1500);  // Stabilize power

  IPAddress ip(10, 2, 0, 188);
  IPAddress gateway(10, 2, 0, 254);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(ip, gateway, subnet);
  WiFi.setAutoReconnect(true);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(WIFI_HOSTNAME);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
                                       "GET, POST, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers",
                                       "Content-Type");

  for (uint8_t i = 0; i < MAX_NETWORKS; i++) {
    wifiMulti.addAP(networks[i][0], networks[i][1]);
  }

  while (wifiMulti.run() != WL_CONNECTED && this->retries <= WIFI_MAX_RETRIES) {
    this->retries += 1;
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) this->connected = true;

  if (this->connected) {
    this->ipAddr = WiFi.localIP().toString();
    Serial.println("IP address: " + this->ipAddr);

    this->startServer();
  }
}

void WifiOut::startServer() {
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/onStart", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json",
                  "{\"setPoint\":" + (String)pidPuller.getSetPoint() +
                      ", \"min\": " + (String)measuring.minRead +
                      ", \"max\":" + (String)measuring.maxRead + "}");
  });

  events.onConnect([](AsyncEventSourceClient* client) {
    client->send("", NULL, millis(), 100);
  });

  server.addHandler(&events);
  server.begin();
}

void WifiOut::putEvent(const char* eventName, String eventData) {
  if (!this->connected) return;

  events.send(eventData.c_str(), eventName, millis());
}

void WifiOut::handleEvents() {
  if (!this->connected) return;

  events.send("ping", NULL, millis());
  events.send(String(getExtrudedLength()).c_str(), "extrudedLength", millis());
  events.send(String(getExtrudedWeight()).c_str(), "extrudedWeight", millis());
  events.send(String(puller.speed).c_str(), "pullerSpeed", millis());
  events.send(String(millis() - millisOffset).c_str(), "time", millis());
}