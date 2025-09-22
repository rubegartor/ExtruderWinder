#include "globals.h"
#include "UI/components/general.h"
#include "pins.h"

Tensioner tensioner;
Puller puller;
Aligner aligner;
Spooler spooler;

Storage storage;
RPCManager rpcManager;
Buzzer buzzer;

float diameter = DIAMETER_DEFAULT;
bool alertsEnabled = ALERT_ENABLED_DEFAULT;
int weightAlertThreshold = WEIGHT_ALERT_DEFAULT;

bool wifiEnabled = WIFI_ENABLED_DEFAULT;

int32_t aligner_to_move = static_cast<int32_t>(STEPS_PER_CM * (diameter / 10.0f));

// Variables de medición recibidas desde M7
float measurementLastRead = 0.0f;
float measurementMinRead = 0.0f;
float measurementMaxRead = 0.0f;

std::vector<PlasticType> plasticTypes = {
  {"PLA", 1.24f},
  {"ABS", 1.02f},
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
  rpcManager.begin();

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
  alertsEnabled = storage.getBool(ALERT_ENABLED_PREF, ALERT_ENABLED_DEFAULT);
  weightAlertThreshold = storage.getFloat(WEIGHT_ALERT_PREF, WEIGHT_ALERT_DEFAULT);
  wifiEnabled = storage.getBool(WIFI_ENABLED_PREF, WIFI_ENABLED_DEFAULT);

  aligner_to_move = static_cast<int32_t>(STEPS_PER_CM * (diameter / 10.0f));
  
  measurementLastRead = 0.00f;
  measurementMinRead = 0.00f;
  measurementMaxRead = 0.00f;
}

void updateDiameter(float value) {
  diameter = value;

  puller.updateSetPoint(diameter);
  aligner_to_move = static_cast<int32_t>(STEPS_PER_CM * (diameter / 10.0f));

  storage.setFloat(DIAMETER_PREF, diameter);

  updateChartLimits();
}

void updateAlertsEnabled(bool value) {
  alertsEnabled = value;
  storage.setBool(ALERT_ENABLED_PREF, alertsEnabled);

  if (!alertsEnabled) {
    buzzer.stop();
  }
}

void updateWeightAlertThreshold(int16_t value) {
  weightAlertThreshold = value;
  storage.setInt(WEIGHT_ALERT_PREF, weightAlertThreshold);
}

void updateWifiEnabled(bool value) {
  wifiEnabled = value;
  storage.setBool(WIFI_ENABLED_PREF, wifiEnabled);
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

  // Si las alertas están habilitadas y el peso supera el umbral, emitir una alerta
  if ((alertsEnabled && weightAlertThreshold > 0) && weight >= weightAlertThreshold) {
    buzzer.beep(BEEP_MEDIUM);
  }

  return weight;
}

void resetMeasurements() {
  puller.resetRevolutionCount();
  buzzer.stop();

  // Resetear las variables locales de medición
  measurementLastRead = 0.0f;
  measurementMinRead = 0.0f;
  measurementMaxRead = 0.0f;

  // Enviar comando de reset al Core M7 a través del RPC
  rpcManager.requestMeasurementReset();
}