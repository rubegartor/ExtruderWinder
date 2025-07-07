#include "IRf.h"
#include "Commons/Commons.h"

RF24 radio(CE, CSN);

const uint64_t pipe = 0xE8E8F0F0E1LL;

void IRf::setup() {
  radio.begin(&SPI1);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_2MBPS);

  radio.openReadingPipe(1, pipe);
  radio.startListening();
}

void IRf::loop(unsigned long interval) {
  unsigned long currentMillis = millis();

  if (currentMillis - this->rf_loop_last_millis >= interval) {
    this->rf_loop_last_millis = currentMillis;

    if (radio.available()) {
      radio.read(&receivedData, sizeof(receivedData));
    }
  }
}