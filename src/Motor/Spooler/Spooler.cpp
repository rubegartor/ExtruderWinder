#include "Spooler.h"
#include "Commons/Commons.h"

const tmc51x0::SpiParameters spi_parameters =
{
  SPI1,
  1000000, // clock_rate
  SPOOLER_CS_PIN // chip_select_pin
};

const tmc51x0::ConverterParameters converter_parameters =
{
  TMC5160_CLK, // clock_frequency_mhz
  800, // microsteps_per_real_unit
};
// external clock is 16MHz
// 200 fullsteps per revolution for many steppers * 256 microsteps per fullstep
// one "real unit" in this example is one rotation of the motor shaft
// rotations/s -> rotations/min
// rotations/(s^2) -> (rotations/min)/s

const tmc51x0::DriverParameters driver_parameters_real =
{
  100, // global_current_scaler (percent)
  35, // run_current (percent)
  45, // hold_current (percent)
  0, // hold_delay (percent)
  15, // pwm_offset (percent)
  5, // pwm_gradient (percent)
  false, // automatic_current_control_enabled
  tmc51x0::FORWARD, // motor_direction
  tmc51x0::NORMAL, // standstill_mode
  tmc51x0::SPREAD_CYCLE, // chopper_mode
  1, // stealth_chop_threshold (rotations/min)
  true, // stealth_chop_enabled
  1, // cool_step_threshold (rotations/min)
  1, // cool_step_min
  0, // cool_step_max
  true, // cool_step_enabled
  20000, // high_velocity_threshold (rotations/min)
  false, // high_velocity_fullstep_enabled
  false, // high_velocity_chopper_switch_enabled
  0, // stall_guard_threshold
  true, // stall_guard_filter_enabled
  true // short_to_ground_protection_enabled
};

const tmc51x0::ControllerParameters controller_parameters_real =
{
  tmc51x0::VELOCITY_POSITIVE, // ramp_mode
  tmc51x0::SOFT, // stop_mode
  0, // max_velocity (rotations/min)
  SPOOLER_MAX_SPEED * 2, // max_acceleration ((rotations/min)/s)
};

void Spooler::setupPID() {
  this->pid = QuickPID(&input, &output, &setPoint, Kp, Ki, Kd, QuickPID::Action::direct);

  this->setPoint = SETPOINT_PID_SPOOLER;
  this->minOutput = 0;
  this->maxOutput = SPOOLER_MAX_SPEED;

  this->pid.SetMode(QuickPID::Control::automatic);
}

void Spooler::setupDriver() {
  this->spooler.setupSpi(spi_parameters);

  this->spooler.converter.setup(converter_parameters);

  tmc51x0::DriverParameters driver_parameters_chip = this->spooler.converter.driverParametersRealToChip(driver_parameters_real);
  this->spooler.driver.setup(driver_parameters_chip);

  tmc51x0::ControllerParameters controller_parameters_chip = this->spooler.converter.controllerParametersRealToChip(controller_parameters_real);
  this->spooler.controller.setup(controller_parameters_chip);

  this->spooler.driver.enable();

  this->spooler.controller.beginRampToZeroVelocity();
  while (!this->spooler.controller.zeroVelocity());
  this->spooler.controller.endRampToZeroVelocity();
  this->spooler.controller.zeroActualPosition();
}

void Spooler::setup() {
  this->setupPID();
  this->setupDriver();
}

void Spooler::loop(unsigned long interval) {
  unsigned long currentMillis = millis();

  if (currentMillis - this->spooler_loop_last_millis >= interval) {
    this->spooler_loop_last_millis = currentMillis;

    this->input = tensioner.distance;

    this->pid.Compute();

    this->speed = map(this->output, MIN_PID_SPOOLER_OUTPUT_LIMIT, MAX_PID_SPOOLER_OUTPUT_LIMIT, this->maxOutput, this->minOutput);

    if (!aligner.isPositioned()) {
      this->spooler.controller.writeMaxVelocity(0);
      return;
    }

    this->spooler.controller.writeMaxVelocity(this->spooler.converter.velocityRealToChip(this->speed));

    if (this->spooler.controller.readActualPosition() >= SPOOLER_ONE_REV_STEPS) {
      this->spooler.controller.zeroActualPosition();
      this->revs++;
    }
  }
}