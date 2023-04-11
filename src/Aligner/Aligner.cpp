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
BlockNot readDiameter(80);

bool homingAligner = false;
bool firstMove = false;
uint16_t lastTotalRevs = 0;

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

int16_t moveStep() {
  int steps = stepsPerCm * (filamentDiameter / 10);

  if (rotaryEncoder.direction == increased) {
    steps = -steps;
  }

  alignerMotor.setAcceleration(20000);
  alignerMotor.setSpeed(200);
  alignerMotor.runToNewPosition(alignerMotor.currentPosition() + steps);

  return alignerMotor.currentPosition();
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
    wifiOut.receive();

    if (!firstMove) {
      alignerMotor.setSpeed(2000);
      alignerMotor.setAcceleration(20000);

      alignerMotor.runToNewPosition(ALIGNER_FIRST_MOVE);

      firstMove = true;
    }

    if (readDiameter.TRIGGERED) {
      measuring.read();
    }

    if (measuring.mode == measuringAutoMode) {
      pullerSpeed = pidPuller.computeSpeed();
    }

    if (updateSummary.TRIGGERED) {
      refreshSummary();
    }

    if (needHome && !homingAligner) {
      alignerMotor.setSpeed(1500);
      homed = goToHome();
      needHome = !homed;

      if (homed) {
        lcdMenu.println("Elige el comienzo de", 1, true);
        lcdMenu.println("     la bobina", 2);

        isStartPosSet = false;
        isEndPosSet = false;
        spoolEndPos = 0;
        homingAligner = true;
      }
    }

    if (!needHome && homed && homingAligner) {
      if (!isStartPosSet) {
        if (rotaryEncoder.changed()) {
          moveStep();
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
          moveStep();
        }

        if (rotaryEncoder.clicked()) {
          isEndPosSet = true;
          spoolEndPos = alignerMotor.currentPosition();

          alignerMotor.setSpeed(1000);
          alignerMotor.runToNewPosition(0);

          homingAligner = false;

          lcdMenu.initSummary(true);
        }
      }
    }

    if (!homingAligner) {
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
          tensioner.getDistance();
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

void resetSpoolerRevs() {
  lastTotalRevs = 0;
  spoolTotalRevs = 0;
}