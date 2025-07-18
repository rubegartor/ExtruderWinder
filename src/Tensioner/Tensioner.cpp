#include "Tensioner.h"
#include "Commons/pins.h"

NewPing sonar(TENSIONER_TRIGGER_PIN, TENSIONER_ECHO_PIN);

void Tensioner::setup() {
}

void Tensioner::execute() {
  this->distance = sonar.ping_cm(TENSIONER_MAX_DISTANCE / 10) * 10;
}