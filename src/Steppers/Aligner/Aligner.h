#pragma once

#include <AccelStepper.h>
#include <TMCStepper.h>

#define ALIGNER_DIR_PIN 5
#define ALIGNER_STEP_PIN 4
#define ALIGNER_CS_PIN 2
#define ALIGNER_STARTUP_MOVE -100
#define ALIGNER_MAX_SPEED 10000

#define ALIGNER_START_POSITION 0

#define ALIGNER_MAX_DISTANCE -5300  // En steps

// Número de lecturas de stallguard a ignorar
#define STALLGUARD_IGNORE 10

// Si el stall value es menor a este número se considera el stall
#define STALLGUARD_THRESHOLD 80

enum MotorDirEnum { forward, backward };

enum AlignerStatus { alignerNoStatus, alignerStart, alignerPositioned };

enum AlignerMoveType { autoAligner, manualAligner };

class Aligner {
 private:
  uint8_t ignoreStallNum;

  bool homed;
  bool needPosition;

  bool isStartPosSet;
  bool isEndPosSet;
  int16_t preStartPos;
  int16_t endPos;

  long lastTotalRevs;

  AlignerMoveType alignerMoveType;

  bool isInPosition();

  uint16_t getStallValue();

  int16_t calculateStepsForNextAlignerMove(AlignerMoveType type);

  void homeProcess();

 public:
  AlignerStatus alignerActualStatus;

  void init();

  void configDriver();

  void run();

  void startAlignerPosition();

  void resetHome();

  bool isPositioned();

  bool isHomed();
};