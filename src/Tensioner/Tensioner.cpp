#include <Commons/Commons.h>
#include <Tensioner/Tensioner.h>

void Tensioner::init() {
  this->firstRead = -1;
  this->secondRead = -1;

  pinMode(TENSIONER_TRIGGER_PIN, OUTPUT);
  pinMode(TENSIONER_ECHO_PIN, INPUT);

  digitalWrite(TENSIONER_TRIGGER_PIN, LOW);
}

long Tensioner::getDistance() {
  digitalWrite(TENSIONER_TRIGGER_PIN, HIGH);
  delayMicroseconds(4);
  digitalWrite(TENSIONER_TRIGGER_PIN, LOW);

  uint16_t pulseWidth = pulseIn(TENSIONER_ECHO_PIN, HIGH);

  float distanceCM = (pulseWidth * TENSIONER_SOUND_SPEED) / 2;

  return (long)(distanceCM * 10);
}

long Tensioner::calcDiffPercent(long valorAnterior, long valorNuevo) {
  if (valorAnterior == 0) return 0;

  return ((valorNuevo - valorAnterior) / valorAnterior) * 100;
}

void Tensioner::run() {
  if (this->firstRead == -1) {
    long distance = this->getDistance();
    this->firstRead = distance <= TENSIONER_MAX_DISTANCE ? distance : 0;
    return;
  }

  if (this->firstRead != -1 && this->secondRead == -1) {
    long distance = this->getDistance();
    this->secondRead = distance <= TENSIONER_MAX_DISTANCE ? distance : 0;
  }

  if (this->firstRead == -1 || this->secondRead == -1) {
    return;
  }

  long difference = this->calcDiffPercent(this->firstRead, this->secondRead);
  if (difference <= TENSIONER_PERTCENT_DIFF) {
    this->lastRead =
        this->firstRead < this->secondRead ? this->firstRead : this->secondRead;
  }

  this->firstRead = -1;
  this->secondRead = -1;
}
