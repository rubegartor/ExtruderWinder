#include "globals.h"
#include "UI/components/general.h"
#include "pins.h"

Measurement measurement;
Tensioner tensioner;
Puller puller;
Aligner aligner;
Spooler spooler;

Storage storage;

float diameter = DIAMETER_DEFAULT;
int32_t aligner_to_move = static_cast<int32_t>(STEPS_PER_CM * (diameter / 10.0f));

std::vector<PlasticType> plasticTypes = {
  {"PLA", 1.24f},
  {"ABS", 1.04f},
  {"PETG", 1.27f},
  {"TPU", 1.20f},
  {"ASA", 1.06f},
  {"PC", 1.20f},
  {"NYLON", 1.15f},
  {"PP", 0.90f},
  {"PET", 1.38f},
  {"PC-ABS", 1.15f}
};

uint8_t selectedPlasticTypeIndex = 0;
int spoolCalibrationCount = 0;

void initGlobals() {
  Serial.begin(115200);
  
  SPI1.begin();

  pinMode(ALIGNER_CS_PIN, OUTPUT);
  pinMode(PULLER_CS_PIN, OUTPUT);
  pinMode(SPOOLER_CS_PIN, OUTPUT);

  digitalWrite(ALIGNER_CS_PIN, HIGH);
  digitalWrite(PULLER_CS_PIN, HIGH);
  digitalWrite(SPOOLER_CS_PIN, HIGH);

  Display.begin();
  TouchDetector.begin();

  diameter = storage.getFloat(DIAMETER_PREF, DIAMETER_DEFAULT);
  selectedPlasticTypeIndex = storage.getInt(PLASTIC_TYPE_PREF, PLASTIC_TYPE_DEFAULT);

  aligner_to_move = static_cast<int32_t>(STEPS_PER_CM * (diameter / 10.0f));
}

void updateDiameter(float value) {
  diameter = value;

  puller.updateSetPoint(diameter);
  aligner_to_move = static_cast<int32_t>(STEPS_PER_CM * (diameter / 10.0f));

  storage.setFloat(DIAMETER_PREF, diameter);

  updateChartLimits();
}

void updatePlasticType(uint8_t index) {
  selectedPlasticTypeIndex = index;
  storage.setInt(PLASTIC_TYPE_PREF, selectedPlasticTypeIndex);
}

float getExtrudedFilamentLength() {
  return (PI * PULLER_DIAM / 1000.0f) * puller.getRevolutionCount();  // En metros
}

float getExtrudedFilamentWeight() {
  const PlasticType& plastic = plasticTypes[selectedPlasticTypeIndex];
  const float density = plastic.density; // g/cm³

  // Diámetro en mm, pásalo a cm
  float radius_cm = (diameter / 2.0f) / 10.0f;
  float area_cm2 = PI * radius_cm * radius_cm; // cm²

  // Longitud en metros, pásala a cm
  float length_cm = getExtrudedFilamentLength() * 100.0f; // metros a cm

  // Volumen en cm³
  float volume_cm3 = area_cm2 * length_cm;

  // Peso en gramos
  float weight = density * volume_cm3;
  return weight;
}