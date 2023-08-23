#pragma once

#include <Arduino.h>

#define TENSIONER_TRIGGER_PIN 40
#define TENSIONER_ECHO_PIN 41

#define TENSIONER_SOUND_SPEED .0343f
#define TENSIONER_MAX_DISTANCE 250
#define TENSIONER_PERTCENT_DIFF 35

class Tensioner {
 private:
  long firstRead;
  long secondRead;

  long getDistance();

  long calcDiffPercent(long valorAnterior, long valorNuevo);

 public:
  long lastRead;

  void init();

  void run();
};