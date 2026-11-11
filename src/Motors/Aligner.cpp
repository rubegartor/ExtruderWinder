#include "Aligner.h"
#include "Commons/pins.h"
#include "Commons/globals.h"
#include "UI/components/general.h"
#include "Stepper.h"

const auto spi_parameters = tmc51x0::SpiParameters{}
  .withSpi(&SPI_INTERFACE)
  .withChipSelectPin(ALIGNER_CS_PIN)
  .withClockRate(TMC5160_SPI_FREQ);

const auto converter_parameters = tmc51x0::ConverterParameters{}
  .withClockFrequencyMHz(12)
  .withMicrostepsPerRealPositionUnit(6400)
  .withSecondsPerRealVelocityUnit(1);

tmc51x0::DriverParameters getDriverParameters(AlignerParameterMode mode) {
  bool isCalibration = (mode == AlignerParameterMode::CALIBRATION);
  return tmc51x0::DriverParameters{}
    .withGlobalCurrentScaler(48)
    .withRunCurrent(isCalibration ? static_cast<uint8_t>(7) : static_cast<uint8_t>(20))
    .withHoldCurrent(isCalibration ? static_cast<uint8_t>(15) : static_cast<uint8_t>(25))
    .withHoldDelay(2)
    .withPwmOffset(30)
    .withPwmGradient(15)
    .withMotorDirection(tmc51x0::ReverseDirection)
    .withStealthChopThreshold(1u)
    .withCoolStepThreshold(1u)
    .withHighVelocityThreshold(10000u)
    .withStallGuardThreshold(static_cast<int8_t>(isCalibration ? 2 : 5))
    .withDcTime(static_cast<uint16_t>(37))
    .withDcStallGuardThreshold(static_cast<uint8_t>(isCalibration ? 2 : 5));
}

tmc51x0::ControllerParameters getControllerParameters(AlignerParameterMode mode) {
  bool isCalibration = (mode == AlignerParameterMode::CALIBRATION);
  return tmc51x0::ControllerParameters{}
    .withRampMode(tmc51x0::PositionMode)
    .withMaxVelocity(static_cast<uint32_t>(isCalibration ? 25 : 50))
    .withMaxAcceleration(300u)
    .withStartVelocity(180u)
    .withStopVelocity(180u)
    .withFirstVelocity(180u)
    .withFirstAcceleration(300u)
    .withMaxDeceleration(500u)
    .withFirstDeceleration(500u)
    .withStallStopEnabled(true);
}

void Aligner::setup(unsigned long interval) {
  setInterval(interval);

  motor.setupSpi(spi_parameters, tmc51x0::Registers::DeviceModel::TMC5160A);
  motor.converter.setup(converter_parameters);

  applyParameters(AlignerParameterMode::NORMAL_OPERATION);
  
  motor.driver.enable();

  motor.controller.beginRampToZeroVelocity();
  unsigned long zeroVelTimeout = millis() + 5000;
  while (!motor.controller.zeroVelocity() && millis() < zeroVelTimeout);
  motor.controller.endRampToZeroVelocity();
  motor.controller.zeroActualPosition();
  // endRampToZeroVelocity() restaura el modo de rampa que LEYÓ del chip: fijar el nuestro
  // (incluye el stop por stall para el homing).
  applyParameters(AlignerParameterMode::NORMAL_OPERATION);
}

void Aligner::execute() {
  switch (checkTmcHealth(motor, guard, TMC_ALIGNER)) {
    case TmcHealth::Glitch:
      return;
    case TmcHealth::Lost:
      // El chip se reinició: XACTUAL vuelve a 0, la referencia espacial se pierde.
      // reinit() lanza re-homing automático preservando la calibración (startPos/endPos).
      reinit();
      return;
    case TmcHealth::Ok:
      break;
  }

  handleStateMachine();

  lastKnownPosition = motor.controller.readActualPosition();
}

void Aligner::loop() {
  processPendingActions();
  handleManualMovement();

  if (shouldExecute()) {
    execute();
  }
}

void Aligner::requestManualMove(int32_t delta) {
  // Llamado desde el hilo de la UI. Solo escribe flags (sin SPI).
  pendingManualMoveDelta = delta;
  pendingManualMove = true;
}

void Aligner::requestResetHome() {
  pendingResetHome = true;
}

void Aligner::requestStartCalibration() {
  pendingStartCalibration = true;
}

void Aligner::processPendingActions() {
  // Se ejecuta dentro del motorTask: aquí es seguro hacer SPI porque ningún
  // otro hilo toca el bus. Procesa las acciones solicitadas desde la UI.
  if (pendingResetHome) {
    pendingResetHome = false;
    resetHome();
  }
  if (pendingStartCalibration) {
    pendingStartCalibration = false;
    startSpoolCalibration();
  }
  if (pendingManualMove) {
    pendingManualMove = false;
    moveTo(pendingManualMoveDelta);
  }
}

void Aligner::handleStateMachine() {
  switch (currentState) {      
    case AlignerState::HOMING:
      handleHoming();
      break;
    case AlignerState::MOVING_TO_HOME:
      handleMovingToHome();
      break;
    case AlignerState::SPOOL_CALIBRATION:
      handleSpoolCalibration();
      break;
    case AlignerState::WAITING:
      handleWaiting();
      break;
    case AlignerState::IDLE:
    case AlignerState::AUTO_MOVE:
      break;
  }
}

// El homing y la calibración fijan el origen y los límites con estos dos flags: se exigen
// dos lecturas seguidas iguales para que un bit corrupto no deje el origen a mitad de recorrido.
bool Aligner::stalled() {
  return motor.driver.stalled() && motor.driver.stalled();
}

bool Aligner::positionReached() {
  return motor.controller.positionReached() && motor.controller.positionReached();
}

void Aligner::handleHoming() {
  if (stalled()) {
    motor.controller.disableStallStop();
    resetPositions();
    waitFor(100, AlignerState::MOVING_TO_HOME);
  } else {
    writeTarget(MAX_ALIGNER_POSITION * 2);
  }
}

void Aligner::handleMovingToHome() {
  writeTarget(-MAX_ALIGNER_POSITION);

  if (positionReached()) {
    if (!homingAfterError) {
      resetPositions();
      writeTarget(MAX_ALIGNER_POSITION / 2);
      currentState = AlignerState::IDLE;
    } else {
      resetPositions();
      writeTarget(getEffectiveStartPos());
      currentState = AlignerState::AUTO_MOVE;
    }
  }
}

void Aligner::handleSpoolCalibration() {
  if (stalled()) {
    motor.controller.disableStallStop();
    
    if (!startPositionSet) {
      setStartPosition();
      waitFor(500, AlignerState::SPOOL_CALIBRATION);
    } else if (!endPositionSet) {
      setEndPosition();
    }
  } else {
    if (startPositionSet && !endPositionSet) {
      motor.controller.enableStallStop();
    }
    if (startPositionSet && endPositionSet && positionReached()) {
      currentState = AlignerState::AUTO_MOVE;
      motor.controller.disableStallStop();
    }
  }
}

void Aligner::handleWaiting() {
  if (millis() - waitStartTime >= waitDuration) {
    currentState = nextStateAfterWait;
  }
}

void Aligner::handleManualMovement() {
  if (millis() - aligner_manual_movement_last_millis >= MANUAL_MOVE_DELAY) {
    if (canMoveRight) {
      moveTo(aligner_to_move);
    } else if (canMoveLeft) {
      moveTo(-aligner_to_move);
    }
    aligner_manual_movement_last_millis = millis();
  }
}

void Aligner::moveTo(int32_t position) {
  int32_t to_go = targetPosition + position;
  int32_t clampedPosition = max(getEffectiveStartPos(), min(to_go, getEffectiveEndPos()));
  writeTarget(clampedPosition);
}

void Aligner::writeTarget(int32_t position) {
  // XTARGET se guarda en RAM en vez de releerlo por SPI: una lectura corrupta
  // en onSpoolerRevolution()/moveTo() mandaba el carro de golpe a un extremo.
  targetPosition = position;
  motor.controller.writeTargetPosition(position);
}

int32_t Aligner::readStoppedPosition() {
  // Motor parado (tras stall): dos lecturas seguidas deben coincidir. Evita fijar
  // un límite de calibración con una lectura SPI corrupta.
  int32_t a = motor.controller.readActualPosition();
  for (uint8_t i = 0; i < 3; i++) {
    int32_t b = motor.controller.readActualPosition();
    if (a == b) break;
    a = b;
  }
  return a;
}

void Aligner::onSpoolerRevolution() {
  if (currentState != AlignerState::AUTO_MOVE) return;

  // Paso base según dirección
  int32_t step = (direction == AlignerDirection::FORWARD) ? aligner_to_move : -aligner_to_move;
  // Escalado con redondeo al entero más cercano para evitar sesgos por truncado
  float scaled = step * 1.35f;
  int32_t stepScaled = static_cast<int32_t>(scaled + (scaled >= 0.0f ? 0.5f : -0.5f));
  int32_t nextPosition = targetPosition + stepScaled;

  int32_t effectiveEndPos = getEffectiveEndPos();
  int32_t effectiveStartPos = getEffectiveStartPos();

  if (nextPosition >= effectiveEndPos) {
    direction = AlignerDirection::BACKWARD;
    // Aterrizar exactamente en el límite derecho
    nextPosition = effectiveEndPos;
  } else if (nextPosition <= effectiveStartPos) {
    direction = AlignerDirection::FORWARD;
    // Aterrizar exactamente en el límite izquierdo
    nextPosition = effectiveStartPos;
  }
  
  // Escribir posición saturada dentro de [start, end]
  writeTarget(max(effectiveStartPos, min(nextPosition, effectiveEndPos)));
}

bool Aligner::isSpoolCalibrated() {
  return startPositionSet && endPositionSet;
}

bool Aligner::isHoming() {
  return currentState == HOMING || currentState == MOVING_TO_HOME;
}

int32_t Aligner::currentPosition() {
  // Devuelve la posición cacheada por execute() para evitar transacciones SPI
  // desde el hilo de la UI (LVGL timer) simultáneas con las del motorTask.
  return lastKnownPosition;
}

void Aligner::resetHome(bool afterError) {
  homingAfterError = afterError;

  if (!afterError) {
    startPositionSet = false;
    endPositionSet = false;
    startPos = 0;
    endPos = MAX_ALIGNER_POSITION;
    startExtension = 0;
    endExtension = 0;
  }

  updateSpoolProgressLimits(getEffectiveStartPos(), getEffectiveEndPos());
  resetPositions();

  writeTarget(MAX_ALIGNER_POSITION);
  motor.controller.enableStallStop();
  currentState = AlignerState::HOMING;
}

void Aligner::startSpoolCalibration() {
  applyParameters(AlignerParameterMode::CALIBRATION);
  motor.controller.enableStallStop();
  writeTarget(-MAX_ALIGNER_POSITION * 2);
  currentState = AlignerState::SPOOL_CALIBRATION;
}

void Aligner::waitFor(unsigned long durationMs, AlignerState nextState) {
  waitStartTime = millis();
  waitDuration = durationMs;
  nextStateAfterWait = nextState;
  currentState = AlignerState::WAITING;
}

void Aligner::applyParameters(AlignerParameterMode mode) {
  motor.driver.setup(motor.converter.driverParametersRealToChip(getDriverParameters(mode)));
  motor.controller.setup(motor.converter.controllerParametersRealToChip(getControllerParameters(mode)));
}

void Aligner::resetPositions() {
  motor.controller.zeroActualPosition();
  writeTarget(0);
}

void Aligner::setStartPosition() {
  startPos = readStoppedPosition();
  startPositionSet = true;

  writeTarget(MAX_ALIGNER_POSITION * 2);
}

void Aligner::setEndPosition() {
  endPos = readStoppedPosition();
  writeTarget(endPos);
  endPositionSet = true;
  applyParameters(AlignerParameterMode::NORMAL_OPERATION);
  writeTarget(startPos);
  updateSpoolProgressLimits(getEffectiveStartPos(), getEffectiveEndPos());
}

void Aligner::addStartExtension() {
  if (isSpoolCalibrated()) {
    startExtension += aligner_to_move;
    updateSpoolProgressLimits(getEffectiveStartPos(), getEffectiveEndPos());
  }
}

void Aligner::removeStartExtension() {
  if (isSpoolCalibrated()) {
    startExtension -= aligner_to_move;
    updateSpoolProgressLimits(getEffectiveStartPos(), getEffectiveEndPos());
  }
}

void Aligner::addEndExtension() {
  if (isSpoolCalibrated()) {
    endExtension += aligner_to_move;
    updateSpoolProgressLimits(getEffectiveStartPos(), getEffectiveEndPos());
  }
}

void Aligner::removeEndExtension() {
  if (isSpoolCalibrated()) {
    endExtension -= aligner_to_move;
    updateSpoolProgressLimits(getEffectiveStartPos(), getEffectiveEndPos());
  }
}

int32_t Aligner::getStartExtensionSteps() {
  return startExtension / aligner_to_move;
}

int32_t Aligner::getEndExtensionSteps() {
  return endExtension / aligner_to_move;
}

int32_t Aligner::getEffectiveStartPos() {
  // startExtension desplaza el límite hacia la izquierda (negativo en posición)
  int32_t s = startPos - startExtension;
  int32_t e = endPos + endExtension;
  return (s <= e) ? s : e; // evitar invertir límites
}

int32_t Aligner::getEffectiveEndPos() {
  int32_t s = startPos - startExtension;
  int32_t e = endPos + endExtension;
  return (e >= s) ? e : s; // evitar invertir límites
} 

void Aligner::reinit() {
  // El chip se reinició y está con sus valores de reset: reescribir toda la config
  // (parámetros normales, por si se estaba calibrando) y re-homing.
  applyParameters(AlignerParameterMode::NORMAL_OPERATION);
  resetHome(true);
}