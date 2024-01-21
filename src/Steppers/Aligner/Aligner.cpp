#include <Arduino.h>
#include <BlockNot.h>
#include <Commons/Commons.h>
#include <Steppers/Aligner/Aligner.h>

TMC2130Stepper driverAligner =
    TMC2130Stepper(ALIGNER_CS_PIN, 0.11f, SPI_MOSI, SPI_MISO, SPI_SCK);

AccelStepper alignerMotor(AccelStepper::DRIVER, ALIGNER_STEP_PIN,
                          ALIGNER_DIR_PIN);

BlockNot homeTimer(5);

AlignerStatus alignerActualStatus = alignerNoStatus;
MotorDirEnum motorDir = forward;

using namespace TMC2130_n;

void Aligner::configDriver() {
  driverAligner.begin();                         // Initiate pins and registers
  driverAligner.toff(4);                         // off time
  driverAligner.blank_time(5);                   // blank time
  driverAligner.rms_current(400);                // 400mAh RMS
  driverAligner.microsteps(ALIGNER_MICROSTEPS);  // 2 microsteps
  driverAligner.TCOOLTHRS(0xFFFFF);              // 20bit max
  driverAligner.THIGH(0);
  driverAligner.semin(5);
  driverAligner.semax(2);
  driverAligner.sedn(0b01);
  driverAligner.sgt(STALLGUARD_SENSITIVITY);
}

void Aligner::init() {
  pinMode(ALIGNER_DIR_PIN, OUTPUT);

  this->configDriver();

  pinMode(ALIGNER_STEP_PIN, OUTPUT);

  alignerMotor.setPinsInverted(true, false, false);

  alignerMotor.setMaxSpeed(ALIGNER_MAX_SPEED);
}

void Aligner::startAlignerPosition() {
  if (alignerMotor.currentPosition() != ALIGNER_START_POSITION) {
    this->resetHome();
  }

  this->isStartPosSet = false;
  this->isEndPosSet = false;
  this->preStartPos = 0;
  this->endPos = 0;
  this->alignerActualStatus = alignerStart;
}

void Aligner::setAlignerPosition() {
  if (this->isStartPosSet && !this->isEndPosSet) {
    this->isEndPosSet = true;
    this->endPos = alignerMotor.currentPosition();
    this->alignerActualStatus = alignerPositioned;

    alignerMotor.moveTo(ALIGNER_START_POSITION);
    alignerMotor.setSpeed(1500);

    communication.sendEvent("maxPosition", (String)this->endPos);
  }

  if (!this->isStartPosSet) {
    this->isStartPosSet = true;
    this->preStartPos = alignerMotor.currentPosition();
    this->startPos = ALIGNER_START_POSITION;
    alignerMotor.setCurrentPosition(ALIGNER_START_POSITION);

    communication.sendEvent("minPosition", (String)this->startPos);
  }
}

void Aligner::resetHome() {
  alignerMotor.setPinsInverted(false, false, false);

  if (alignerMotor.currentPosition() >= this->startPos) {
    this->invertedPins = false;
  } else if (motorDir == backward) {
    this->invertedPins = false;
  } else {
    this->invertedPins = true;
  }

  this->homed = false;
  this->ignoreStallNum = 0;
}

void Aligner::moveTo(long pos) {
  bool canMove = false;
  long stepsToGo = alignerMotor.currentPosition() + pos;

  if (!this->isPositioned() && stepsToGo >= ALIGNER_START_POSITION &&
      abs(stepsToGo + this->preStartPos) <= abs(ALIGNER_MAX_DISTANCE)) {
    canMove = true;
  } else if (this->isPositioned() && stepsToGo >= this->startPos &&
      abs(stepsToGo) <= abs(this->endPos)) {
    canMove = true;
  }

  if (canMove) {
    alignerMotor.moveTo(stepsToGo);
    alignerMotor.setSpeed(1250);
  }
}

long Aligner::getCurrentPosition() {
  return alignerMotor.currentPosition();
}

bool Aligner::isInPosition() {
  return alignerMotor.currentPosition() == alignerMotor.targetPosition();
}

void Aligner::run() {
  if (measuring.autoStopStatus == autoStopTriggered) return;

  if (!this->isHomed()) {
    homeProcess();
  }

  if (isReady()) {
    if (spooler.totalRevs > this->lastTotalRevs) {
      this->lastTotalRevs = spooler.totalRevs;
      alignerMotor.moveTo(this->calculateStepsForNextAlignerMove());
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
long Aligner::calculateStepsForNextAlignerMove() {
  long stepsToGo = (STEPS_PER_CM * (filamentDiameter / 10));

  if (alignerMotor.currentPosition() >= this->endPos) {
    motorDir = backward;
  }

  if (alignerMotor.currentPosition() <= this->startPos) {
    motorDir = forward;
  }

  if (motorDir == backward) {
    stepsToGo = -(stepsToGo);
  }

  return alignerMotor.currentPosition() + stepsToGo;
}

uint16_t Aligner::getStallValue() {
  DRV_STATUS_t drv_status{0};
  drv_status.sr = driverAligner.DRV_STATUS();

  return drv_status.sg_result;
}

void Aligner::homeProcess() {
  alignerMotor.setPinsInverted(this->invertedPins, false, false);

  if (homeTimer.TRIGGERED && !this->homed) {
    alignerMotor.setSpeed(1500);

    if (this->ignoreStallNum < STALLGUARD_IGNORE) {
      this->ignoreStallNum++;
    } else if (this->getStallValue() < STALLGUARD_THRESHOLD) {
      this->homed = true;
      alignerMotor.setCurrentPosition(ALIGNER_START_POSITION);
      alignerMotor.setPinsInverted(true, false, false);
    }
  }
}

bool Aligner::isHomed() { return this->homed; }

bool Aligner::isPositioned() { return isStartPosSet && isEndPosSet; }

bool Aligner::isOverTempWarn() {
  DRV_STATUS_t drv_status{0};
  drv_status.sr = driverAligner.DRV_STATUS();

  return drv_status.otpw;
}

bool Aligner::isOverTemp() {
  DRV_STATUS_t drv_status{0};
  drv_status.sr = driverAligner.DRV_STATUS();

  return drv_status.ot;
}

bool Aligner::isEnabled(bool restartIfDisabled) {
  bool enabled = driverAligner.isEnabled();

  if (restartIfDisabled && !enabled) this->configDriver();

  return enabled;
}

bool Aligner::isUnderVoltage() { return driverAligner.uv_cp(); }