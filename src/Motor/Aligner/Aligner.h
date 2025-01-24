#pragma once

#include <TMC51X0.hpp>

#define ALIGNER_CS_PIN 9

#define STEPS_PER_CM 52350

#define MAX_ALIGNER_POSITION (STEPS_PER_CM * 13)

enum MotorDirection {
  FORWARD,
  BACKWARD
};

enum State {
  HOMING,
  WAITING_FOR_HOME,
  MOVING_TO_TARGET,
  AUTO_MOVE,
  IDLE
};

class Aligner
{
  private:
    TMC51X0 aligner;
    State currentState = HOMING;
    MotorDirection motorDirection = BACKWARD;

    uint32_t lastRevs = 0;
    bool startPositionReached = false;
    unsigned long aligner_manual_movement_last_millis = 0;
  public:
    int32_t startPos;
    int32_t endPos = MAX_ALIGNER_POSITION;

    bool canMoveRight = false;
    bool canMoveLeft = false;

    void setup();
    void loop();

    int32_t nextMovePosition();
    int32_t currentPosition();
    void moveTo(int32_t position);
    void stop(int32_t correction = 0);
    void resetHome();
    void setStartPosition();
    void setEndPosition();
    bool isHoming();
    bool isPositioned();
};