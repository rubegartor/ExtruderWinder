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

void Aligner::setup() {
  motor.setupSpi(spi_parameters, tmc51x0::Registers::DeviceModel::TMC5160A);
  motor.converter.setup(converter_parameters);

  applyParameters(AlignerParameterMode::NORMAL_OPERATION);
  
  motor.driver.enable();

  motor.controller.beginRampToZeroVelocity();
  while (!motor.controller.zeroVelocity());
  motor.controller.endRampToZeroVelocity();
  motor.controller.zeroActualPosition();
  motor.controller.enableStallStop();
}

void Aligner::execute() {
  tmc51x0::HealthStatus health = motor.readHealthStatus();

  if (health.reset || health.mirror_resync_required) {
    // El chip se reinició: XACTUAL vuelve a 0, la referencia espacial se pierde.
    // reinit() restaura la configuración del driver y lanza re-homing automático,
    // preservando la calibración del spool (startPos/endPos).
    reinit();
    return;
  }

  if (!health.communication_ok || health.driver_error || health.charge_pump_undervoltage) {
    // Otro error de hardware sin reset de posición: restaurar solo la configuración.
    motor.notePossibleMirrorDrift();
    motor.recoverIfNeeded();
  }

  handleStateMachine();

  if (!this->enabled()) {
    this->reinit();
  }

  lastKnownPosition = motor.controller.readActualPosition();
}

void Aligner::loop() {
  handleManualMovement();

  if (shouldExecute()) {
    execute();
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

void Aligner::handleHoming() {
  if (motor.driver.stalled()) {
    motor.controller.disableStallStop();
    resetPositions();
    waitFor(100, AlignerState::MOVING_TO_HOME);
  } else {
    motor.controller.writeTargetPosition(MAX_ALIGNER_POSITION * 2);
  }
}

void Aligner::handleMovingToHome() {
  motor.controller.writeTargetPosition(-MAX_ALIGNER_POSITION);
  
  if (motor.controller.positionReached()) {
    if (!homingAfterError) {
      resetPositions();
      motor.controller.writeTargetPosition(MAX_ALIGNER_POSITION / 2);
      currentState = AlignerState::IDLE;
    } else {
      resetPositions();
      motor.controller.writeTargetPosition(getEffectiveStartPos());
      currentState = AlignerState::AUTO_MOVE;
    }
  }
}

void Aligner::handleSpoolCalibration() {
  if (motor.driver.stalled()) {
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
    if (startPositionSet && endPositionSet && motor.controller.positionReached()) {
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
  int32_t current_position = motor.controller.readTargetPosition();
  int32_t to_go = current_position + position;
  int32_t clampedPosition = max(getEffectiveStartPos(), min(to_go, getEffectiveEndPos()));
  motor.controller.writeTargetPosition(clampedPosition);
}

void Aligner::onSpoolerRevolution() {
  if (currentState != AlignerState::AUTO_MOVE) return;

  // Paso base según dirección
  int32_t step = (direction == AlignerDirection::FORWARD) ? aligner_to_move : -aligner_to_move;
  // Escalado con redondeo al entero más cercano para evitar sesgos por truncado
  float scaled = step * 1.35f;
  int32_t stepScaled = static_cast<int32_t>(scaled + (scaled >= 0.0f ? 0.5f : -0.5f));
  int32_t nextPosition = motor.controller.readTargetPosition() + stepScaled;

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
  motor.controller.writeTargetPosition(max(effectiveStartPos, min(nextPosition, effectiveEndPos)));
}

bool Aligner::isSpoolCalibrated() {
  return startPositionSet && endPositionSet;
}

bool Aligner::isHoming() {
  return currentState == HOMING || currentState == MOVING_TO_HOME;
}

int32_t Aligner::currentPosition() {
  return motor.controller.readActualPosition();
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

  motor.controller.writeTargetPosition(MAX_ALIGNER_POSITION);
  motor.controller.enableStallStop();
  currentState = AlignerState::HOMING;
}

void Aligner::startSpoolCalibration() {
  applyParameters(AlignerParameterMode::CALIBRATION);
  motor.controller.enableStallStop();
  motor.controller.writeTargetPosition(-MAX_ALIGNER_POSITION * 2);
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
  motor.controller.zeroTargetPosition();
}

void Aligner::setStartPosition() {
  startPos = motor.controller.readActualPosition();
  startPositionSet = true;

  motor.controller.writeTargetPosition(MAX_ALIGNER_POSITION * 2);
}

void Aligner::setEndPosition() {
  endPos = motor.controller.readActualPosition();
  motor.controller.writeTargetPosition(endPos);
  endPositionSet = true;
  applyParameters(AlignerParameterMode::NORMAL_OPERATION);
  motor.controller.writeTargetPosition(startPos);
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
  // No llamar a SPI1.end()/begin(): mataría la comunicación del Puller y Spooler también.
  // Solo reinicializar este driver TMC manteniendo el bus SPI activo.
  motor.reinitialize();
  delay(10);

  applyParameters(AlignerParameterMode::NORMAL_OPERATION);
  motor.driver.enable();

  disabledCount = 0;
  resetHome(true);
}

bool Aligner::enabled() {
  using namespace tmc51x0;

  tmc51x0::Registers::Chopconf chopConf;
  chopConf.raw = motor.registers.read(Registers::ChopconfAddress);

  if (chopConf.toff() != 0) {
    disabledCount = 0;
    return true;
  }

  // Requerir fallos consecutivos antes de considerar el driver deshabilitado.
  // Un solo fallo SPI por ruido no debe lanzar un reinit completo en producción.
  disabledCount++;
  return disabledCount < ENABLED_FAIL_THRESHOLD;
}