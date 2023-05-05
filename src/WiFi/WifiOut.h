#pragma once

#include <Arduino.h>

class WifiOut {
 private:
  bool connected;

  void processWifiMessages(String header, String data);

 public:
  String ipAddr;

  bool isConnected();
  void put(String header, String info, String data = "");
  void receive();
};