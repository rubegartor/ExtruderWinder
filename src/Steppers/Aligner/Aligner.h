#pragma once

#include <AccelStepper.h>
#include <TMCStepper.h>

#define ALIGNER_SPI

#define ALIGNER_DIR_PIN 38
#define ALIGNER_STEP_PIN 16
#define ALIGNER_CS_PIN 10
#define ALIGNER_STARTUP_MOVE -100
#define ALIGNER_MAX_SPEED 10000

#define STEPS_PER_CM 525

#define ALIGNER_MICROSTEPS 2

#define ALIGNER_START_POSITION 0

#define ALIGNER_MAX_DISTANCE 5300  // En steps

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
  bool invertedPins;
  bool needPosition;

  bool isStartPosSet;
  bool isEndPosSet;
  int16_t preStartPos;

  AlignerMoveType alignerMoveType;

  void configDriver();

  bool isInPosition();

  uint16_t getStallValue();

  long calculateStepsForNextAlignerMove(AlignerMoveType type);

  void homeProcess();

 public:
  AlignerStatus alignerActualStatus;

  long lastTotalRevs;

  int16_t startPos;

  int16_t endPos;

  void init();

  void run();

  void startAlignerPosition();

  void resetHome();

  void moveTo(long pos);

  bool isPositioned();

  bool isHomed();

  bool isOverTempWarn();

  bool isOverTemp();

  bool isEnabled(bool restartIfDisabled = false);

  bool isUnderVoltage();
};