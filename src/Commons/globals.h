#pragma once

#include <vector>

#include "Tensioner/Tensioner.h"
#include "Motors/Puller.h"
#include "Motors/Aligner.h"
#include "Motors/Spooler.h"
#include "Commons/Storage.h"
#include "Buzzer/Buzzer.h"
#include "Commons/RPCManager.h"

#define DIAMETER_PREF "diameter"
#define DIAMETER_DEFAULT 1.75f

#define PLASTIC_TYPE_PREF "plasticType"
#define PLASTIC_TYPE_DEFAULT 0 // PLA

#define ALERT_ENABLED_PREF "alertEnabled"
#define ALERT_ENABLED_DEFAULT false

#define WEIGHT_ALERT_PREF "weightAlert"
#define WEIGHT_ALERT_DEFAULT 0

#define WIFI_ENABLED_PREF "wifiEnabled"
#define WIFI_ENABLED_DEFAULT false

extern Tensioner tensioner;
extern Puller puller;
extern Aligner aligner;
extern Spooler spooler;

extern Storage storage;
extern RPCManager rpcManager;
extern Buzzer buzzer;

extern float diameter;
extern bool alertsEnabled;
extern int weightAlertThreshold;

extern bool wifiEnabled;

extern int32_t aligner_to_move;

// Variables de medición recibidas desde M7
extern float measurementLastRead;
extern float measurementMinRead;
extern float measurementMaxRead;

struct PlasticType {
  String name;
  float density; // g/cm³
};

extern std::vector<PlasticType> plasticTypes;
extern uint8_t selectedPlasticTypeIndex;
extern int spoolCalibrationCount;


void initGlobals();
void updateDiameter(float value);
void updateAlertsEnabled(bool value);
void updateWeightAlertThreshold(int16_t value);
void updateFilamentAlertThreshold(float value);

void updateWifiEnabled(bool value);

void updatePlasticType(uint8_t index);

void resetMeasurements();

float getExtrudedFilamentLength();
float getExtrudedFilamentWeight();