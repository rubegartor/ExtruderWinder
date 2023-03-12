#include <AccelStepper.h>
#include <Aligner/Aligner.h>
#include <Arduino.h>
#include <BlockNot.h>
#include <Commons/Commons.h>
#include <LCD/LCDMenu.h>

enum MotorDirEnum { forward, backward };

AccelStepper alignerMotor(AccelStepper::DRIVER, ALIGNER_STEP_PIN,
                          ALIGNER_DIR_PIN);

BlockNot readDistance(200);
BlockNot updateSummary(500);

bool firstMove = false;
uint16_t lastTotalRevs = 0;

bool isHomingAligner = false;
bool isStartPosSet = false;
bool isEndPosSet = false;
int16_t spoolEndPos = 0;

MotorDirEnum motorDir = forward;

bool goToHome() {
  bool homeSensor = digitalRead(ALIGNER_HOME_SENSOR_PIN);

  if (homeSensor == LOW) {
    alignerMotor.runSpeed();
  }

  return homeSensor;
}

void onRotaryEncoderChange() {
  int steps = stepsPerCm * (filamentDiameter / 10);

  if (rotaryEncoder.direction == increased) {
    steps = -steps;
  }

  alignerMotor.setAcceleration(20000);
  alignerMotor.setSpeed(200);
  alignerMotor.runToNewPosition(alignerMotor.currentPosition() + steps);
}

void moveAligner() {
  int stepsToGo = -(stepsPerCm * (filamentDiameter / 10));

  if (motorDir == backward) {
    stepsToGo = -(stepsToGo);
  }

  alignerMotor.setSpeed(2000);
  alignerMotor.setAcceleration(20000);

  alignerMotor.runToNewPosition(alignerMotor.currentPosition() +
                                stepsToGo);  // Blocking call

  if (alignerMotor.currentPosition() <= spoolEndPos) {
    motorDir = backward;
  }

  // 0 is always start position of spool
  if (alignerMotor.currentPosition() >= 0) {
    motorDir = forward;
  }
}

void refreshSummary() {
  if (lcdMenu.inSummary) {
    lcdMenu.initSummary();
  }
}

void aTask(void *pvParameters) {
  alignerMotor.setMaxSpeed(ALIGNER_MAX_SPEED);
  alignerMotor.setSpeed(1500);

  lcdMenu.initSummary(true);

  for (;;) {
    if (!firstMove) {
      alignerMotor.setSpeed(2000);
      alignerMotor.setAcceleration(20000);

      alignerMotor.runToNewPosition(ALIGNER_FIRST_MOVE);

      firstMove = true;
    }

    calibration.read();

    if (updateSummary.TRIGGERED) {
      refreshSummary();
    }

    if (needHome && !isHomingAligner) {
      alignerMotor.setSpeed(1500);
      homed = goToHome();
      needHome = !homed;

      if (homed) {
        lcdMenu.println("Elige el comienzo de", 1, true);
        lcdMenu.println("     la bobina", 2);

        isStartPosSet = false;
        isEndPosSet = false;
        spoolEndPos = 0;
        isHomingAligner = true;
      }
    }

    if (!needHome && homed && isHomingAligner) {
      if (!isStartPosSet) {
        if (rotaryEncoder.changed()) {
          onRotaryEncoderChange();
        }

        if (rotaryEncoder.clicked()) {
          alignerMotor.setCurrentPosition(0);
          isStartPosSet = true;

          lcdMenu.println(" Elige el final de", 1, true);
          lcdMenu.println("     la bobina", 2);
        }
      }

      if (isStartPosSet && !isEndPosSet) {
        if (rotaryEncoder.changed()) {
          onRotaryEncoderChange();
        }

        if (rotaryEncoder.clicked()) {
          isEndPosSet = true;
          spoolEndPos = alignerMotor.currentPosition();

          alignerMotor.setSpeed(1000);
          alignerMotor.moveTo(0);
          alignerMotor.runToPosition();

          isHomingAligner = false;

          lcdMenu.initSummary(true);
        }
      }
    }

    if (!isHomingAligner) {
      // Cuando se haya completado una revolución de la bobina se debe mover el
      // alineador
      if (spoolTotalRevs != lastTotalRevs) {
        lastTotalRevs = spoolTotalRevs;
        moveAligner();
      }

      if (lcdMenu.menuPosition == pullerSpeedOption) {
        lcdMenu.checkLCDButtons();
      }

      if (homed && isEndPosSet) {
        if (readDistance.TRIGGERED) {
          actualDistance = tensioner.getDistance();
        }
      }

      if (rotaryEncoder.changed()) {
        lcdMenu.onREncoderChange(rotaryEncoder);
      }

      if (rotaryEncoder.clicked()) {
        lcdMenu.onREncoderClick(rotaryEncoder);
      }
    }

    watchDogFeed();
  }
}