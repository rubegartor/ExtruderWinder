#pragma once

#include <Arduino.h>

class WifiOut {
 public:
  String ipAddr;

  void put(String header, String info, String data = "");
  void receive();
};