#include "Commons.h"
#include "Screen/components/general.h"

Aligner aligner;
Puller puller;
Spooler spooler;
Tensioner tensioner;
Measurement measurement;
IRf rf;
Storage storage;

float diameter = DIAMETER_DEFAULT;

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

int selectedPlasticTypeIndex = 0;

int32_t aligner_to_move;
int16_t aligner_left_pos, aligner_right_pos;

std::vector<MotorError> motorErrors;

void initCommons() {
  pinMode(ALIGNER_CS_PIN, OUTPUT);
  pinMode(PULLER_CS_PIN, OUTPUT);
  pinMode(SPOOLER_CS_PIN, OUTPUT);

  digitalWrite(ALIGNER_CS_PIN, HIGH);
  digitalWrite(PULLER_CS_PIN, HIGH);
  digitalWrite(SPOOLER_CS_PIN, HIGH);

  aligner_to_move = static_cast<int32_t>(STEPS_PER_CM * (diameter / 10.0f));
  diameter = storage.getFloat(DIAMETER_PREF, DIAMETER_DEFAULT);
  selectedPlasticTypeIndex = storage.getInt(PLASTIC_TYPE_PREF, PLASTIC_TYPE_DEFAULT);
}

void motorWatchdog() {
  if (!aligner.enabled()) {
    addMotorError({0, millis(), aligner.drvStatusBytes()});

    aligner.reinit();
  }

  if (!puller.enabled()) {
    addMotorError({1, millis(), puller.drvStatusBytes()});

    puller.reinit();
  }

  if (!spooler.enabled()) {
    addMotorError({2, millis(), spooler.drvStatusBytes()});

    spooler.reinit();
  }
}

void addMotorError(MotorError error) {
  motorErrors.push_back(error);
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
  return (PI * PULLER_DIAM / 1000.0f) * puller.revs;  // En metros
}

float getExtrudedFilamentWeight() {
  const PlasticType& plastic = plasticTypes[selectedPlasticTypeIndex];

  const float density = plastic.density;
  float radius_mm = diameter / 2.0f;
  float radius_cm = radius_mm / 10.0f;
  float area_cm2 = PI * radius_cm * radius_cm;
  float factor = density * area_cm2 * 100.0f; // gramos por metro

  return getExtrudedFilamentLength() * factor;
}