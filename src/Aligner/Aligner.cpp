#include <AccelStepper.h>
#include <Aligner/Aligner.h>
#include <Arduino.h>
#include <BlockNot.h>
#include <Commons/Commons.h>
#include <LCD/LCDMenu.h>
#include <TMCStepper.h>
#include <Winder/Winder.h>

using namespace TMC2130_n;

enum MotorDirEnum { forward, backward };

enum AlignerStatus { alignerNoStatus, alignerStart, alignerPositioned };

TMC2130Stepper driverAligner = TMC2130Stepper(ALIGNER_CS_PIN, R_SENSE);

AccelStepper alignerMotor(AccelStepper::DRIVER, ALIGNER_STEP_PIN,
                          ALIGNER_DIR_PIN);

BlockNot homeTimer(5);
BlockNot readDistance(150);
BlockNot updateSummary(500);
BlockNot readDiameter(80);
BlockNot extruderResume(1000);

bool homed;
bool onStartupMove;
uint8_t ignoreStallNum;
AlignerStatus alignerActualStatus = alignerNoStatus;

uint16_t lastTotalRevs = 0;

bool startPosMessageShowed;
bool isStartPosSet;
bool isEndPosSet;
int16_t spoolEndPos = 0;

MotorDirEnum motorDir = forward;

void configAlignerDriver() {
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

void startAlignerPosition() {
  isStartPosSet = false;
  isEndPosSet = false;
  startPosMessageShowed = false;
  spoolEndPos = 0;
  alignerActualStatus = alignerStart;

  toggleAlignerTestRun();
}

void resetHome() {
  homed = false;
  ignoreStallNum = 0;
}

/**
 * Método que mueve el alineador "un paso" para seleccionar el inicio y fin,
 * establece la dirección segun incremente o decrezca el rotary encoder
 */
int16_t _moveStep() {
  int steps = stepsPerCm * (filamentDiameter / 10);

  if (rotaryEncoder.direction == increased) {
    steps = -steps;
  }

  long toGo = alignerMotor.currentPosition() + steps;

  if (toGo <= 0 && abs(toGo) <= abs(ALIGNER_MAX_DISTANCE)) {
    alignerMotor.setAcceleration(20000);
    alignerMotor.setSpeed(200);
    alignerMotor.runToNewPosition(toGo);
  }

  return alignerMotor.currentPosition();
}

/**
 * Método que mueve el alineador automaticamente cuando se completa una
 * revolucion del winder, corrige automaticamente la dirección
 */
void _moveAligner() {
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

  // 0 es siempre la posicion iniciar del alineador
  if (alignerMotor.currentPosition() >= 0) {
    motorDir = forward;
  }
}

void _refreshSummary() {
  if (lcdMenu.inSummary) {
    lcdMenu.initSummary();
  }
}

uint16_t _getStallValue() {
  DRV_STATUS_t drv_status{0};
  drv_status.sr = driverAligner.DRV_STATUS();

  return drv_status.sg_result;
}

void _homeProcess() {
  if (homeTimer.TRIGGERED && !homed) {
    alignerMotor.setSpeed(1500);

    if (ignoreStallNum < STALLGUARD_IGNORE) {
      ignoreStallNum++;
    } else {
      if (_getStallValue() < STALLGUARD_THRESHOLD) {
        homed = true;
        alignerMotor.setCurrentPosition(0);
      }
    }
  }

  if (!homed) {
    alignerMotor.runSpeed();
  }
}

void _LCDActions() {
  if (rotaryEncoder.changed()) {
    lcdMenu.onREncoderChange(rotaryEncoder);
  }

  if (rotaryEncoder.clicked()) {
    lcdMenu.onREncoderClick(rotaryEncoder);
  }

  if (lcdMenu.menuPosition == pullerSpeedOption) {
    lcdMenu.checkLCDButtons();
  }
}

void _wifiOutSender() {
  if (wifiOut.isConnected()) {
    wifiOut.put("Extruder", "ExtrudedLength", (String)getExtrudedLength());
    wifiOut.put("Extruder", "ExtrudedWeight", (String)getExtrudedWeight());
    wifiOut.put("Extruder", "Time", (String)(millis() - millisOffset));
    wifiOut.put("Extruder", "PullerSpeed", (String)pullerSpeed);
    wifiOut.put("Extruder", "SetPoint", (String)pidPuller.getSetPoint());
  }
}

// TODO: Rotar el spooler mientras se selecciona el inicio y fin del
// posicionador
void _positionAligner() {
  if (!isStartPosSet) {
    if (rotaryEncoder.changed()) {
      _moveStep();
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
      _moveStep();
    }

    if (rotaryEncoder.clicked()) {
      isEndPosSet = true;
      toggleAlignerTestRun();
      resetSpoolerRevs();
      spoolEndPos = alignerMotor.currentPosition();

      alignerMotor.setSpeed(1000);
      alignerMotor.runToNewPosition(0);

      alignerActualStatus = alignerPositioned;

      lcdMenu.initSummary(true);
    }
  }
}

void _doStartupMove() {
  if (!onStartupMove) {
    alignerMotor.setCurrentPosition(0);
    alignerMotor.setAcceleration(10000);
    alignerMotor.setSpeed(1500);
    alignerMotor.runToNewPosition(ALIGNER_STARTUP_MOVE);
    onStartupMove = true;
  }
}

void aTask(void *pvParameters) {
  alignerMotor.setMaxSpeed(ALIGNER_MAX_SPEED);
  alignerMotor.setSpeed(1500);

  for (;;) {
    _doStartupMove();
    _homeProcess();

    // ------------------------------------------

    wifiOut.receive();

    if (isHomed() && readDiameter.TRIGGERED) {
      measuring.read();
    }

    if (isHomed() && measuring.mode == measuringAutoMode) {
      pullerSpeed = pidPuller.computeSpeed();
    }

    if (alignerActualStatus != alignerStart) {
      if (isHomed()) {
        _LCDActions();
      }

      if (isHomed() && updateSummary.TRIGGERED) {
        _refreshSummary();
      }

      if (isHomed() && extruderResume.TRIGGERED) {
        _wifiOutSender();
      }
    }

    if (isHomed() && alignerActualStatus == alignerStart && !isPositioned()) {
      if (!startPosMessageShowed) {
        lcdMenu.println("Elige el comienzo de", 1, true);
        lcdMenu.println("     la bobina", 2);

        startPosMessageShowed = true;
      }

      _positionAligner();
    }

    if (isReady()) {
      // Cuando se haya completado una revolución de la bobina se debe mover el
      // alineador
      if (spoolTotalRevs != lastTotalRevs) {
        lastTotalRevs = spoolTotalRevs;
        _moveAligner();
      }

      if (readDistance.TRIGGERED) {
        tensioner.getDistance();
      }
    }

    watchDogFeed();
  }
}

void resetSpoolerRevs() {
  lastTotalRevs = 0;
  spoolTotalRevs = 0;
}

bool isHomed() { return homed; }

bool isPositioned() { return isStartPosSet && isEndPosSet; }