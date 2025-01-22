#include "Aligner.h"
#include "Commons/Commons.h"

const tmc51x0::SpiParameters spi_parameters =
{
  SPI1,
  1000000, // clock_rate
  ALIGNER_CS_PIN // chip_select_pin
};

const tmc51x0::ConverterParameters converter_parameters =
{
  TMC5160_CLK, // clock_frequency_mhz
  6400 // microsteps_per_real_unit
};
// external clock is 16MHz
// 200 fullsteps per revolution for many steppers * 256 microsteps per fullstep
// 51200 microsteps per revolution / 360 degrees per revolution ~= 142 microsteps per degree
// one "real unit" in this example is one degree of rotation

const tmc51x0::DriverParameters driver_parameters_real =
{
  100, // global_current_scaler (percent)
  20, // run_current (percent)
  50, // hold_current (percent)
  5, // hold_delay (percent)
  15, // pwm_offset (percent)
  5, // pwm_gradient (percent)
  false, // automatic_current_control_enabled
  tmc51x0::REVERSE, // motor_direction
  tmc51x0::NORMAL, // standstill_mode
  tmc51x0::SPREAD_CYCLE, // chopper_mode
  1, // stealth_chop_threshold (degrees/s)
  true, // stealth_chop_enabled
  1, // cool_step_threshold (degrees/s)
  1, // cool_step_min
  0, // cool_step_max
  false, // cool_step_enabled
  100, // high_velocity_threshold (degrees/s)
  false, // high_velocity_fullstep_enabled
  false, // high_velocity_chopper_switch_enabled
  1, // stall_guard_threshold
  false, // stall_guard_filter_enabled
  true, // short_to_ground_protection_enabled
  3, // enabled_toff
  tmc51x0::CLOCK_CYCLES_36, // comparator_blank_time
  37, // dc_time
  3 // dc_stall_guard_threshold
};

const tmc51x0::ControllerParameters controller_parameters_real =
{
  tmc51x0::POSITION, // ramp_mode
  tmc51x0::HARD, // stop_mode
  80, // max_velocity (degrees/s)
  300, // max_acceleration ((degrees/s)/s)
  180, // start_velocity (degrees/s)
  180, // stop_velocity (degrees/s)
  180, // first_velocity (degrees/s)
  300, // first_acceleration ((degrees/s)/s)
  500, // max_deceleration ((degrees/s)/s)
  500, // first_deceleration ((degrees/s)/s)
  0, // zero_wait_duration (milliseconds)
  false // stall_stop_enabled
};

const tmc51x0::ControllerParameters controller_parameters_real_position =
{
  tmc51x0::POSITION, // ramp_mode
  tmc51x0::HARD, // stop_mode
  3000, // max_velocity (degrees/s)
  20000000, // max_acceleration ((degrees/s)/s)
  3000, // start_velocity (degrees/s)
  3000, // stop_velocity (degrees/s)
  3000, // first_velocity (degrees/s)
  20000000, // first_acceleration ((degrees/s)/s)
  20000000, // max_deceleration ((degrees/s)/s)
  20000000, // first_deceleration ((degrees/s)/s)
  0, // zero_wait_duration (milliseconds)
  false // stall_stop_enabled
};

const tmc51x0::HomeParameters home_parameters_real =
{
  12, // run_current (percent)
  20, // hold_current (percent)
  360, // target_position (degrees)
  40, // velocity (degrees/s)
  400, // acceleration ((degrees/s)/s)
  100 // zero_wait_duration (milliseconds)
};

const tmc51x0::StallParameters stall_parameters_cool_step_real =
{
  tmc51x0::DC_STEP, // stall_mode
  8, // stall_guard_threshold
  2 // cool_step_threshold (degrees/s)
};

tmc51x0::ControllerParameters controller_parameters_chip;
tmc51x0::HomeParameters home_parameters_chip;
tmc51x0::StallParameters stall_parameters_cool_step_chip;

void Aligner::setup() {
  aligner.setupSpi(spi_parameters);

  aligner.converter.setup(converter_parameters);

  tmc51x0::DriverParameters driver_parameters_chip = aligner.converter.driverParametersRealToChip(driver_parameters_real);
  aligner.driver.setup(driver_parameters_chip);

  controller_parameters_chip = aligner.converter.controllerParametersRealToChip(controller_parameters_real);
  aligner.controller.setup(controller_parameters_chip);

  home_parameters_chip = aligner.converter.homeParametersRealToChip(home_parameters_real);
  stall_parameters_cool_step_chip = aligner.converter.stallParametersRealToChip(stall_parameters_cool_step_real);

  aligner.driver.enable();

  aligner.controller.beginRampToZeroVelocity();
  while (not aligner.controller.zeroVelocity());
  aligner.controller.endRampToZeroVelocity();
}

int32_t Aligner::nextMovePosition() {
  int32_t current_position_chip = this->aligner.controller.readActualPosition();
  int32_t steps_to_go = to_move * 2;

  if (current_position_chip >= this->endPos) {
    this->motorDirection = BACKWARD;
  }

  if (current_position_chip <= 0) {
    this->motorDirection = FORWARD;
  }

  if (this->motorDirection == BACKWARD) {
    steps_to_go = -(steps_to_go);
  }

  return steps_to_go;
}

void Aligner::loop() {
  switch (currentState) {
    case HOMING:
      aligner.beginHomeToStall(home_parameters_chip, stall_parameters_cool_step_chip);
      currentState = WAITING_FOR_HOME;
      break;

    case WAITING_FOR_HOME:
      if (aligner.homed()) {
        aligner.endHome();
        currentState = MOVING_TO_TARGET;
      }
      break;

    case MOVING_TO_TARGET:
      aligner.controller.writeTargetPosition(-MAX_ALIGNER_POSITION);

      if (aligner.controller.positionReached()) {
        aligner.controller.writeActualPosition(0);
        aligner.controller.writeTargetPosition(0);

        aligner.controller.setup(aligner.converter.controllerParametersRealToChip(controller_parameters_real_position));
        currentState = IDLE;
      }
      break;
    case AUTO_MOVE:
      if (!this->startPositionReached && this->aligner.controller.positionReached()) {
        this->startPositionReached = true;
      }

      if (this->startPositionReached && this->lastRevs != spooler.revs) {
        this->moveTo(nextMovePosition());
        this->lastRevs = spooler.revs;
      }
      break;
    case IDLE:
      if (millis() - aligner_manual_movement_last_millis >= 50) {
        if (this->canMoveRight) {
          this->moveTo(to_move);
        } else if (this->canMoveLeft) {
          this->moveTo(-to_move);
        }

        aligner_manual_movement_last_millis = millis();
      }
      break;
  }
}

int32_t Aligner::currentPosition() {
  return this->aligner.controller.readActualPosition();
}

uint32_t last_moveto_millis = 0;

void Aligner::moveTo(int32_t position) {
  unsigned long current_millis = millis();
  int32_t current_position = this->aligner.controller.readTargetPosition();
  int32_t to_go = current_position + position;

  if (to_go <= 0) {
    this->aligner.controller.writeTargetPosition(0);
  } else if (to_go >= this->endPos) {
    this->aligner.controller.writeTargetPosition(this->endPos);
  } else {
    this->aligner.controller.writeTargetPosition(to_go);
  }

  last_moveto_millis = current_millis;
}

void Aligner::stop(int32_t correction) {
  this->aligner.controller.writeTargetPosition(this->aligner.controller.readActualPosition() - this->aligner.converter.positionRealToChip(correction));
}

void Aligner::resetHome() {
  aligner.controller.setup(aligner.converter.controllerParametersRealToChip(controller_parameters_real));
  this->currentState = HOMING;
}

void Aligner::setStartPosition() {
  this->aligner.controller.writeActualPosition(0);
  this->aligner.controller.writeTargetPosition(0);
}

void Aligner::setEndPosition() {
  this->endPos = this->aligner.controller.readActualPosition();
  this->aligner.controller.writeTargetPosition(0);

  this->currentState = AUTO_MOVE;
}

bool Aligner::isPositioned() {
  return this->startPositionReached && this->currentState == AUTO_MOVE;
}