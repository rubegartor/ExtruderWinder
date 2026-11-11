#include "Tensioner.h"
#include "Commons/pins.h"
#include "Commons/EventLog.h"

NewPing sonar(TENSIONER_TRIGGER_PIN, TENSIONER_ECHO_PIN);

void Tensioner::setup(unsigned long interval) {
  setInterval(interval);
}

void Tensioner::execute() {
  unsigned long cm = sonar.ping_cm(TENSIONER_MAX_DISTANCE / 10);

  // NO_ECHO (0) es "eco perdido / fuera de rango", no "distancia 0". Tratarlo
  // como 0 caía en TENSIONER_MIN_HARD_LIMIT y frenaba el spooler en seco por cada
  // eco perdido. Se mantiene la última lectura; solo si el sensor deja de responder
  // de forma continuada se fuerza 0 (parada, fail-safe).
  if (cm != NO_ECHO) {
    this->distance = cm * 10;
    missedEchoes = 0;
  } else if (missedEchoes < TENSIONER_MAX_MISSED_ECHOES) {
    if (++missedEchoes == TENSIONER_MAX_MISSED_ECHOES) logEvent(SRC_SYS, EV_TENSIONER_LOST);
  } else {
    this->distance = 0;
  }
}