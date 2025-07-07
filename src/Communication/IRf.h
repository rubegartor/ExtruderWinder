#pragma once

#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

#define CE 2
#define CSN 3

class IRf {
  private:
    unsigned long rf_loop_last_millis = 0;
  public:
    struct DataPacket {
      float motorCurrent = 0.0f;
      uint16_t motorSpeed = 0;
      int16_t temps[3] = {0, 0, 0};
    } receivedData;

    void setup();
    void loop(unsigned long interval = 0);
};
