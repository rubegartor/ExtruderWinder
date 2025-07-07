#include "Aligner.h"
#include "Commons/Commons.h"
#include "Screen/components/general.h"

const tmc51x0::SpiParameters spi_parameters =
{
  .spi_ptr = &SPI1,
  .clock_rate = 1000000,
  .chip_select_pin = ALIGNER_CS_PIN
};

const tmc51x0::ConverterParameters converter_parameters =
{
  .clock_frequency_mhz = TMC5160_CLK_FREQ,
  .microsteps_per_real_position_unit = 6400
};

const tmc51x0::DriverParameters driver_parameters_real =
{
  .global_current_scaler = 100,
  .run_current = 20,
  .hold_current = 50,
  .hold_delay = 5,
  .pwm_offset = 15,
  .pwm_gradient = 5,
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
  .high_velocity_threshold = 100,
  .high_velocity_fullstep_enabled = false,
  .high_velocity_chopper_switch_enabled = false,
  .stall_guard_threshold = 1,
  .stall_guard_filter_enabled = false,
  .short_to_ground_protection_enabled = true,
  .enabled_toff = 3,
  .comparator_blank_time = tmc51x0::ClockCycles36,
  .dc_time = 37,
  .dc_stall_guard_threshold = 3
};

const tmc51x0::DriverParameters driver_parameters_real_spool =
{
  .global_current_scaler = 50,
  .run_current = 4,
  .hold_current = 8,
  .hold_delay = 5,
  .pwm_offset = 15,
  .pwm_gradient = 5,
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
  .high_velocity_threshold = 100,
  .high_velocity_fullstep_enabled = false,
  .high_velocity_chopper_switch_enabled = false,
  .stall_guard_threshold = 3,
  .stall_guard_filter_enabled = true,
  .short_to_ground_protection_enabled = true,
  .enabled_toff = 3,
  .comparator_blank_time = tmc51x0::ClockCycles36,
  .dc_time = 0,
  .dc_stall_guard_threshold = 3
};

const tmc51x0::ControllerParameters controller_parameters_real =
{
  .ramp_mode = tmc51x0::PositionMode,
  .stop_mode = tmc51x0::HardMode,
  .max_velocity = 80,
  .max_acceleration = 300,
  .start_velocity = 180,
  .stop_velocity = 180,
  .first_velocity = 180,
  .first_acceleration = 300,
  .max_deceleration = 500,
  .first_deceleration = 500,
  .zero_wait_duration = 0,
  .stall_stop_enabled = false
};

const tmc51x0::ControllerParameters controller_parameters_real_position =
{
  .ramp_mode = tmc51x0::PositionMode,
  .stop_mode = tmc51x0::HardMode,
  .max_velocity = 3000,
  .max_acceleration = 20000000,
  .start_velocity = 3000,
  .stop_velocity = 3000,
  .first_velocity = 3000,
  .first_acceleration = 20000000,
  .max_deceleration = 20000000,
  .first_deceleration = 20000000,
  .zero_wait_duration = 0,
  .stall_stop_enabled = false
};

const tmc51x0::ControllerParameters controller_parameters_real_spool =
{
  .ramp_mode = tmc51x0::PositionMode,
  .stop_mode = tmc51x0::HardMode,
  .max_velocity = 8,
  .max_acceleration = 8,
  .start_velocity = 8,
  .stop_velocity = 8,
  .first_velocity = 8,
  .first_acceleration = 8,
  .max_deceleration = 8,
  .first_deceleration = 8,
  .zero_wait_duration = 0,
  .stall_stop_enabled = true
};

const tmc51x0::HomeParameters home_parameters_real =
{
  .run_current = 12,
  .hold_current = 20,
  .target_position = 360,
  .velocity = 40,
  .acceleration = 400,
  .zero_wait_duration = 0
};

const tmc51x0::StallParameters stall_parameters_real =
{
  .stall_guard_threshold = 2,
  .cool_step_threshold = 2
};

tmc51x0::DriverParameters driver_parameters_chip;
tmc51x0::ControllerParameters controller_parameters_chip;
tmc51x0::DriverParameters driver_parameters_spool_chip;
tmc51x0::ControllerParameters controller_parameters_spool_chip;

void Aligner::setupHomeParameters() {
  tmc51x0::HomeParameters home_parameters_chip = aligner.converter.homeParametersRealToChip(home_parameters_real);
  tmc51x0::StallParameters stall_parameters_chip = aligner.converter.stallParametersRealToChip(stall_parameters_real);

  this->setupNormalParameters();
  aligner.beginHomeToStall(home_parameters_chip, stall_parameters_chip);
}

void Aligner::setupNormalParameters() {
  aligner.driver.setup(driver_parameters_chip);
  aligner.controller.setup(controller_parameters_chip);
}

void Aligner::setupPositionParameters() {
  aligner.driver.setup(driver_parameters_chip);
  aligner.controller.setup(controller_parameters_chip);
}

void Aligner::setupHomingSpoolParameters() {
  aligner.driver.setup(driver_parameters_spool_chip);
  aligner.controller.setup(controller_parameters_spool_chip);
}

void Aligner::setup() {
  aligner.setupSpi(spi_parameters);
  aligner.converter.setup(converter_parameters);

  driver_parameters_chip = aligner.converter.driverParametersRealToChip(driver_parameters_real);
  controller_parameters_chip = aligner.converter.controllerParametersRealToChip(controller_parameters_real);

  driver_parameters_spool_chip = aligner.converter.driverParametersRealToChip(driver_parameters_real_spool);
  controller_parameters_spool_chip = aligner.converter.controllerParametersRealToChip(controller_parameters_real_spool);

  setupHomeParameters();

  aligner.driver.enable();

  aligner.controller.beginRampToZeroVelocity();
  while (not aligner.controller.zeroVelocity());
  aligner.controller.endRampToZeroVelocity();
}

int32_t Aligner::nextMovePosition() {
  int32_t current_position_chip = this->aligner.controller.readActualPosition();
  int32_t steps_to_go = aligner_to_move * 1.25;

  if (current_position_chip >= this->endPos) {
    this->motorDirection = MotorDirection::BACKWARD;
  }

  if (current_position_chip <= this->startPos) {
    this->motorDirection = MotorDirection::FORWARD;
  }

  if (this->motorDirection == MotorDirection::BACKWARD) {
    steps_to_go = -(steps_to_go);
  }

  return steps_to_go;
}

void Aligner::loop() {
  switch (currentState) {
    case HOMING:
      this->setupHomeParameters();
      currentState = WAITING_FOR_HOME;
      break;

    case WAITING_FOR_HOME:
      if (aligner.homed()) {
        aligner.endHome();
        aligner_left_pos = 0;
        aligner_right_pos = 0;
      
        this->currentState = MOVING_TO_TARGET;
      }
      break;

    case SPOOL_CALIBRATION:
      aligner.controller.enableStallStop();

      if (!this->startPosSpoolHomed) {
        aligner.controller.writeTargetPosition(-MAX_ALIGNER_POSITION);
      } else {
        aligner.controller.writeTargetPosition(MAX_ALIGNER_POSITION);
      }

      if (aligner.driver.stalled()) {
        if (!this->startPosSpoolHomed) {
          this->startPosSpoolHomed = true;

          aligner.controller.writeActualPosition(0);
          aligner.controller.writeTargetPosition(0);
          this->startPos = aligner_to_move;
        } else {       
          this->endPos = aligner.controller.readActualPosition() - aligner_to_move;
          aligner.controller.writeTargetPosition(this->startPos);
          this->setupNormalParameters();
          this->currentState = AUTO_MOVE;

          updateSpoolProgressLimits(this->startPos, this->endPos);
          break;
        }

        waitAfterStallMillis = millis();
        aligner.controller.disableStallStop();
        this->currentState = WAIT_AFTER_STALL;
      }
      break;

    case WAIT_AFTER_STALL:
      if (millis() - waitAfterStallMillis >= ALIGNER_STALL_WAIT) {
        this->currentState = SPOOL_CALIBRATION;
      }
      break;

    case MOVING_TO_TARGET:
      aligner.controller.writeTargetPosition(-(MAX_ALIGNER_POSITION));

      if (aligner.controller.positionReached()) {
        aligner.controller.writeActualPosition(0);
        aligner.controller.writeTargetPosition(0);

        currentState = IDLE;
      }
      break;

    case AUTO_MOVE:
      if (this->lastRevs != spooler.revs) {
        this->moveTo(nextMovePosition());
        this->lastRevs = spooler.revs;
      }
      break;

    case IDLE:
      break;
  }

  if (millis() - aligner_manual_movement_last_millis >= 25) {
    if (this->canMoveRight) {
      this->moveTo(aligner_to_move);
    } else if (this->canMoveLeft) {
      this->moveTo(-aligner_to_move);
    }

    aligner_manual_movement_last_millis = millis();
  }
}

int32_t Aligner::currentPosition() {
  return this->aligner.controller.readActualPosition();
}

void Aligner::moveTo(int32_t position) {
  int32_t current_position = this->aligner.controller.readTargetPosition();
  int32_t to_go = current_position + position;

  if (to_go <= this->startPos) {
    this->aligner.controller.writeTargetPosition(this->startPos);
  } else if (to_go >= this->endPos) {
    this->aligner.controller.writeTargetPosition(this->endPos);
  } else {
    this->aligner.controller.writeTargetPosition(to_go);
  }
}

void Aligner::resetHome() {
  setupHomeParameters();

  this->startPos = 0;
  this->endPos = MAX_ALIGNER_POSITION;

  updateSpoolProgressLimits(this->startPos, this->endPos);

  this->startPosSpoolHomed = false;
  this->calibratedSpool = false;
  this->currentState = HOMING;
}

void Aligner::startSpoolCalibration() {  
  setupHomingSpoolParameters();

  this->waitAfterStallMillis = 0;
  this->startPosSpoolHomed = false;
  this->currentState = SPOOL_CALIBRATION;
}

bool Aligner::isHoming() {
  return this->currentState == HOMING || this->currentState == WAITING_FOR_HOME;
}

bool Aligner::isPositioned() {
  return this->currentState == AUTO_MOVE;
}

bool Aligner::enabled() {
  using namespace tmc51x0;

  tmc51x0::Registers::Chopconf chopConf;
  chopConf.bytes = this->aligner.registers.read(Registers::ChopconfAddress);

  return chopConf.toff != 0;
}

bool Aligner::drvErr() {
  using namespace tmc51x0;

  tmc51x0::Registers::Gstat gstat;
  gstat.bytes = this->aligner.registers.read(Registers::GstatAddress);

  return gstat.drv_err;
}

uint32_t Aligner::drvStatusBytes() {
  using namespace tmc51x0;

  tmc51x0::Registers::DrvStatus drv_status;
  drv_status.bytes = this->aligner.registers.read(Registers::DrvStatusAddress);

  return drv_status.bytes;
}

void Aligner::reinit() {
  digitalWrite(ALIGNER_CS_PIN, HIGH);

  this->aligner.reinitialize();
}