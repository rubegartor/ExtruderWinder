#pragma once

#include "Motor/Aligner/Aligner.h"
#include "Motor/Puller/Puller.h"
#include "Motor/Spooler/Spooler.h"
#include "Tensioner/Tensioner.h"
#include "Measurement/Measurement.h"
#include "Communication/IRf.h"
#include "Storage.h"
#include <vector>

#define TMC5160_CLK_FREQ 12 // MHz

#define STEPPER_DEFAULT_STEPS 200

#define DIAMETER_PREF "diameter"

#define DIAMETER_DEFAULT 1.75f

#define PLASTIC_TYPE_PREF "plasticType"

#define PLASTIC_TYPE_DEFAULT 0 // PLA

extern Aligner aligner;
extern Puller puller;
extern Spooler spooler;
extern Tensioner tensioner;
extern Measurement measurement;
extern IRf rf;
extern Storage storage;

extern float diameter;

struct PlasticType {
  String name;
  float density; // g/cm³
};

extern std::vector<PlasticType> plasticTypes;
extern int selectedPlasticTypeIndex;

extern int32_t aligner_to_move;
extern int16_t aligner_left_pos, aligner_right_pos;

struct MotorError {
  uint8_t motorId;
  unsigned long timestamp;
  uint32_t drvStatusBytes;
};

extern std::vector<MotorError> motorErrors;

void initCommons();

void motorWatchdog();

void addMotorError(MotorError error);

void updateDiameter(float value);
void updatePlasticType(uint8_t index);

float getExtrudedFilamentLength();
float getExtrudedFilamentWeight();