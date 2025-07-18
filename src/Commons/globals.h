#pragma once

#include <vector>

#include "Measurement/Measurement.h"
#include "Tensioner/Tensioner.h"
#include "Motors/Puller.h"
#include "Motors/Aligner.h"
#include "Motors/Spooler.h"
#include "Commons/Storage.h"

#define DIAMETER_PREF "diameter"
#define DIAMETER_DEFAULT 1.75f

#define PLASTIC_TYPE_PREF "plasticType"
#define PLASTIC_TYPE_DEFAULT 0 // PLA

extern Measurement measurement;
extern Tensioner tensioner;
extern Puller puller;
extern Aligner aligner;
extern Spooler spooler;

extern Storage storage;

extern float diameter;
extern int32_t aligner_to_move;

struct PlasticType {
  String name;
  float density; // g/cm³
};

extern std::vector<PlasticType> plasticTypes;
extern uint8_t selectedPlasticTypeIndex;
extern int spoolCalibrationCount;


void initGlobals();
void updateDiameter(float value);
void updatePlasticType(uint8_t index);

float getExtrudedFilamentLength();
float getExtrudedFilamentWeight();