#include "Commons.h"
#include <Wire.h>

Aligner aligner;
Puller puller;
Spooler spooler;
Tensioner tensioner;
Measurement measurement;
EEPROM_Preferences preferences(0x50, EEPROM_24LC256);

float diameter;
float autostop;

int32_t aligner_to_move;
int16_t aligner_left_pos, aligner_right_pos;

void initCommons() {
  pinMode(ALIGNER_CS_PIN, OUTPUT);
  pinMode(PULLER_CS_PIN, OUTPUT);
  pinMode(SPOOLER_CS_PIN, OUTPUT);

  digitalWrite(ALIGNER_CS_PIN, HIGH);
  digitalWrite(PULLER_CS_PIN, HIGH);
  digitalWrite(SPOOLER_CS_PIN, HIGH);

  preferences.begin();

  diameter = preferences.getFloat(DIAMETER_PREF, DIAMETER_DEFAULT);
  autostop = preferences.getFloat(AUTOSTOP_PREF, AUTOSTOP_DEFAULT);

  aligner_to_move = static_cast<int32_t>(STEPS_PER_CM * (diameter / 10.0f));
}

void updateDiameter(float value) {
  diameter = value;
  preferences.writeFloat(DIAMETER_PREF, value);

  puller.updateSetPoint(value);
  aligner_to_move = static_cast<int32_t>(STEPS_PER_CM / value);
}

void updateAutostop(float value) {
  autostop = value;
  preferences.writeFloat(AUTOSTOP_PREF, value);
}

char* getTime(unsigned long millis) {
  int seconds = (millis / 1000) % 60;
  int minutes = (millis / (1000 * 60)) % 60;
  int hours = (millis / (1000 * 60 * 60)) % 24;

  static char time_str[9];
  sprintf(time_str, "%02d:%02d:%02d", hours, minutes, seconds);
  return time_str;
}

float getExtrudedLength() {
  float circ = PI * ((float)PULLER_DIAM / 1000.0f);
  return circ * (float)puller.revs;
}

float getExtrudedWeight() {
  return getExtrudedLength() * 2.98f;
}