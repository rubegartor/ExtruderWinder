#include "Aligner.h"
#include "Commons/pins.h"
#include "Commons/globals.h"
#include "UI/components/general.h"
#include "Stepper.h"

const tmc51x0::SpiParameters spi_parameters = {
  .spi_ptr = &SPI_INTERFACE,
  .clock_rate = TMC5160_SPI_FREQ,
  .chip_select_pin = ALIGNER_CS_PIN
};

const tmc51x0::ConverterParameters converter_parameters = {
  .clock_frequency_mhz = 12,
  .microsteps_per_real_position_unit = 6400
};

tmc51x0::DriverParameters getDriverParameters(AlignerParameterMode mode) {
  bool isCalibration = (mode == AlignerParameterMode::CALIBRATION);
  return {
    .global_current_scaler = 48,
    .run_current = isCalibration ? static_cast<uint8_t>(7) : static_cast<uint8_t>(20),
    .hold_current = isCalibration ? static_cast<uint8_t>(15) : static_cast<uint8_t>(25),
    .hold_delay = 2,
    .pwm_offset = 30,
    .pwm_gradient = 15,
    .automatic_current_control_enabled = false,
    .motor_direction = tmc51x0::ReverseDirection,
    .standstill_mode = tmc51x0::NormalMode,
    .chopper_mode = tmc51x0::SpreadCycleMode,
    .stealth_chop_threshold = 1,
    .stealth_chop_enabled = true,
    .cool_step_threshold = 1,
    .cool_step_min = 1,
    .cool_step_max = 0,
    .cool_step_enabled = false,
    .high_velocity_threshold = 10000,
    .high_velocity_fullstep_enabled = false,
    .high_velocity_chopper_switch_enabled = false,
    .stall_guard_threshold = static_cast<int8_t>(isCalibration ? 2 : 5),
    .stall_guard_filter_enabled = false,
    .short_to_ground_protection_enabled = true,
    .enabled_toff = 3,
    .comparator_blank_time = tmc51x0::ClockCycles36,
    .dc_time = 37,
    .dc_stall_guard_threshold = static_cast<uint8_t>(isCalibration ? 2 : 5)
  };
}

tmc51x0::ControllerParameters getControllerParameters(AlignerParameterMode mode) {
  bool isCalibration = (mode == AlignerParameterMode::CALIBRATION);
  return {
    .ramp_mode = tmc51x0::PositionMode,
    .stop_mode = tmc51x0::HardMode,
    .max_velocity = static_cast<uint32_t>(isCalibration ? 25 : 50),
    .max_acceleration = 300,
    .start_velocity = 180,
    .stop_velocity = 180,
    .first_velocity = 180,
    .first_acceleration = 300,
    .max_deceleration = 500,
    .first_deceleration = 500,
    .zero_wait_duration = 0,
    .stall_stop_enabled = true
  };
}

void Aligner::setup() {
  motor.setupSpi(spi_parameters);
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
    } else if (!endPositionSet) {
      setEndPosition();
    }
  } else {
    if (startPositionSet && endPositionSet && motor.controller.positionReached()) {
      currentState = AlignerState::AUTO_MOVE;
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
  motor.controller.enableStallStop(); 
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
  SPI1.end();
  delay(10);
  SPI1.begin();

  delay(10);
  motor.reinitialize();
  delay(10);

  resetHome(true);
}

bool Aligner::enabled() {
  using namespace tmc51x0;

  tmc51x0::Registers::Chopconf chopConf;
  chopConf.bytes = motor.registers.read(Registers::ChopconfAddress);

  return chopConf.toff != 0;
}