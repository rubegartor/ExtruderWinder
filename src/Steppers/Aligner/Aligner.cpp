#include <Arduino.h>
#include <BlockNot.h>
#include <Commons/Commons.h>
#include <LCD/LCDMenu.h>
#include <Steppers/Aligner/Aligner.h>

TMC2130Stepper driverAligner = TMC2130Stepper(ALIGNER_CS_PIN, 0.11f);

AccelStepper alignerMotor(AccelStepper::DRIVER, ALIGNER_STEP_PIN,
                          ALIGNER_DIR_PIN);

BlockNot homeTimer(5);

AlignerStatus alignerActualStatus = alignerNoStatus;
MotorDirEnum motorDir = forward;

using namespace TMC2130_n;

void Aligner::configDriver() {
  driverAligner.begin();             // Initiate pins and registers
  driverAligner.toff(4);             // off time
  driverAligner.blank_time(24);      // blank time
  driverAligner.rms_current(400);    // 400mAh RMS
  driverAligner.microsteps(2);       // 2 microsteps
  driverAligner.TCOOLTHRS(0xFFFFF);  // 20bit max
  driverAligner.THIGH(0);
  driverAligner.semin(5);
  driverAligner.semax(2);
  driverAligner.sedn(0b01);
  driverAligner.sgt(STALLGUARD_SENSITIVITY);
}

void Aligner::init() {
  pinMode(ALIGNER_DIR_PIN, OUTPUT);
  pinMode(ALIGNER_STEP_PIN, OUTPUT);

  digitalWrite(ALIGNER_DIR_PIN, LOW);

  alignerMotor.setMaxSpeed(ALIGNER_MAX_SPEED);
}

void Aligner::startAlignerPosition() {
  this->isStartPosSet = false;
  this->isEndPosSet = false;
  this->preStartPos = 0;
  this->endPos = 0;
  this->alignerActualStatus = alignerStart;

  lcdMenu.println(" Elige el inicio de", 1, true);
  lcdMenu.println("     la bobina", 2);
}

void Aligner::resetHome() {
  this->homed = false;
  this->ignoreStallNum = 0;
}

bool Aligner::isInPosition() {
  return alignerMotor.currentPosition() == alignerMotor.targetPosition();
}

void Aligner::run() {
  if (measuring.autoStopStatus == autoStopTriggered) return;

  if (!this->isHomed()) {
    homeProcess();
  }

  if (this->homed && this->alignerActualStatus == alignerStart) {
    if (this->isInPosition()) {
      if (rotaryEncoder.changed()) {
        alignerMotor.moveTo(
            this->calculateStepsForNextAlignerMove(manualAligner));
        alignerMotor.setSpeed(800);
      }

      if (rotaryEncoder.clicked()) {
        if (this->isStartPosSet && !this->isEndPosSet) {
          this->isEndPosSet = true;
          this->endPos = alignerMotor.currentPosition();
          this->alignerActualStatus = alignerPositioned;

          alignerMotor.moveTo(ALIGNER_START_POSITION);
          alignerMotor.setSpeed(1500);

          lcdMenu.initSummary(true);
        }

        if (!this->isStartPosSet) {
          this->isStartPosSet = true;
          this->preStartPos = alignerMotor.currentPosition();
          alignerMotor.setCurrentPosition(ALIGNER_START_POSITION);

          lcdMenu.println(" Elige el final de", 1, true);
          lcdMenu.println("     la bobina", 2);
        }
      }
    }
  }

  if (isReady()) {
    if (spooler.totalRevs != this->lastTotalRevs) {
      this->lastTotalRevs = spooler.totalRevs;
      alignerMotor.moveTo(this->calculateStepsForNextAlignerMove(autoAligner));
      alignerMotor.setSpeed(1250);
    }
  }

  if (!this->homed) {
    alignerMotor.runSpeed();
  } else {
    alignerMotor.runSpeedToPosition();
  }
}

/**
 * Método que mueve el alineador automaticamente cuando se completa una
 * revolucion del winder, corrige automaticamente la dirección
 */
int16_t Aligner::calculateStepsForNextAlignerMove(AlignerMoveType type) {
  int16_t stepsToGo = (stepsPerCm * (filamentDiameter / 10));

  if (type == manualAligner) {
    if (rotaryEncoder.direction == increased) stepsToGo = -stepsToGo;

    stepsToGo = alignerMotor.currentPosition() + stepsToGo;

    if (stepsToGo <= ALIGNER_START_POSITION &&
        abs(stepsToGo + this->preStartPos) <= abs(ALIGNER_MAX_DISTANCE)) {
      return stepsToGo;
    }

    return alignerMotor.currentPosition();
  } else {
    if (alignerMotor.currentPosition() <= this->endPos) {
      motorDir = backward;
    }

    if (alignerMotor.currentPosition() >= ALIGNER_START_POSITION) {
      motorDir = forward;
    }

    if (motorDir == forward) {
      stepsToGo = -(stepsToGo);
    }

    return alignerMotor.currentPosition() + stepsToGo;
  }
}

uint16_t Aligner::getStallValue() {
  DRV_STATUS_t drv_status{0};
  drv_status.sr = driverAligner.DRV_STATUS();

  return drv_status.sg_result;
}

void Aligner::homeProcess() {
  if (homeTimer.TRIGGERED && !this->homed) {
    alignerMotor.setSpeed(1500);

    if (this->ignoreStallNum < STALLGUARD_IGNORE) {
      this->ignoreStallNum++;
    } else {
      if (this->getStallValue() < STALLGUARD_THRESHOLD) {
        this->homed = true;
        alignerMotor.setCurrentPosition(ALIGNER_START_POSITION);
      }
    }
  }
}

bool Aligner::isHomed() { return this->homed; }

bool Aligner::isPositioned() { return isStartPosSet && isEndPosSet; }