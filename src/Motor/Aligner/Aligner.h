#pragma once

#include <TMC51X0.hpp>

#define ALIGNER_CS_PIN 9

#define STEPS_PER_CM 52350

#define MAX_ALIGNER_POSITION (STEPS_PER_CM * 13)

#define ALIGNER_STALL_WAIT 100 // milliseconds

enum MotorDirection {
  FORWARD,
  BACKWARD
};

enum State {
  HOMING,
  WAITING_FOR_HOME,
  SPOOL_CALIBRATION,
  WAIT_AFTER_STALL,
  MOVING_TO_TARGET,
  AUTO_MOVE,
  IDLE
};

class Aligner
{
  private:
    TMC51X0 aligner;
    State currentState = HOMING;
    MotorDirection motorDirection = MotorDirection::BACKWARD;

    uint32_t lastRevs = 0;
    unsigned long aligner_manual_movement_last_millis = 0;
    unsigned long waitAfterStallMillis = 0;

    void setupHomeParameters();
    void setupNormalParameters();
    void setupPositionParameters();
    void setupHomingSpoolParameters();
    int32_t nextMovePosition();

    bool startPosSpoolHomed = false;
    bool calibratedSpool = false;
  public:
    int32_t startPos;
    int32_t endPos = MAX_ALIGNER_POSITION;

    bool canMoveRight = false;
    bool canMoveLeft = false;

    void setup();
    void loop();

    int32_t currentPosition();
    void moveTo(int32_t position);
    void resetHome();
    void startSpoolCalibration();
    void setEndPosition();
    bool isHoming();
    bool isPositioned();
    bool enabled();
    bool drvErr();
    uint32_t drvStatusBytes();
    void reinit();
};