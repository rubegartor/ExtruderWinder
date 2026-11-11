#pragma once

#include <vector>

#include "Tensioner/Tensioner.h"
#include "Motors/Puller.h"
#include "Motors/Aligner.h"
#include "Motors/Spooler.h"
#include "Commons/Storage.h"
#include "Buzzer/Buzzer.h"
#include "Measurement/Measurement.h"

#define DIAMETER_PREF "diameter"
#define DIAMETER_DEFAULT 1.75f
#define DIAMETER_MIN 0.50f
#define DIAMETER_MAX 3.00f

extern Tensioner&   tensioner;
extern Puller&      puller;
extern Aligner&     aligner;
extern Spooler&     spooler;
extern Measurement& measurement;
extern Storage&     storage;
extern Buzzer&      buzzer;

#define PLASTIC_TYPE_PREF "plasticType"
#define PLASTIC_TYPE_DEFAULT 0 // PLA

#define ALERT_ENABLED_PREF "alertEnabled"
#define ALERT_ENABLED_DEFAULT false

#define WEIGHT_ALERT_PREF "weightAlert"
#define WEIGHT_ALERT_DEFAULT 0


// Flag para notificar al hilo principal que hay un nuevo valor de medición para el gráfico
extern volatile bool g_newMeasurementReady;


extern float diameter;
extern bool  alertsEnabled;
extern int   weightAlertThreshold;

extern volatile int32_t aligner_to_move;

extern float measurementLastRead;
extern float measurementMinRead;
extern float measurementMaxRead;

struct PlasticType {
  String name;
  float  density; // g/cm³
};

extern std::vector<PlasticType> plasticTypes;
extern uint8_t selectedPlasticTypeIndex;
extern int     spoolCalibrationCount;


void initGlobals();
void updateDiameter(float value);
void updateAlertsEnabled(bool value);
void updateWeightAlertThreshold(int16_t value);
void updateFilamentAlertThreshold(float value);

void updatePlasticType(uint8_t index);

void resetMeasurements();
void resetMinMax();

float getExtrudedFilamentLength();
float getExtrudedFilamentWeight();

