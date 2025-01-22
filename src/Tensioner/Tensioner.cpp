#include "Tensioner.h"

NewPing sonar(TENSIONER_TRIGGER_PIN, TENSIONER_ECHO_PIN);

void Tensioner::setup() {
}

void Tensioner::loop(unsigned long interval) {
  unsigned long currentMillis = millis();

  if (currentMillis - this->tensioner_loop_last_millis >= interval) {
    this->tensioner_loop_last_millis = currentMillis;
    
    this->distance = sonar.ping_cm() * 10;
  }
}