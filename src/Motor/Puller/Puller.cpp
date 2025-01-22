#include "Puller.h"
#include "Commons/Commons.h"

const tmc51x0::SpiParameters spi_parameters =
{
  SPI1,
  1000000, // clock_rate
  PULLER_CS_PIN // chip_select_pin
};

const tmc51x0::ConverterParameters converter_parameters =
{
  TMC5160_CLK, // clock_frequency_mhz
  STEPPER_DEFAULT_STEPS * PULLER_MICROSTEPS, // microsteps_per_real_unit
  60 // seconds_per_real_velocity_unit
};
// external clock is 16MHz
// 200 fullsteps per revolution for many steppers * 256 microsteps per fullstep
// one "real unit" in this example is one rotation of the motor shaft
// rotations/s -> rotations/min
// rotations/(s^2) -> (rotations/min)/s

const tmc51x0::DriverParameters driver_parameters_real =
{
  100, // global_current_scaler (percent)
  25, // run_current (percent)
  45, // hold_current (percent)
  0, // hold_delay (percent)
  15, // pwm_offset (percent)
  5, // pwm_gradient (percent)
  false, // automatic_current_control_enabled
  tmc51x0::REVERSE, // motor_direction
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
  tmc51x0::HARD, // stop_mode
  0, // max_velocity (rotations/min)
  1000000, // max_acceleration ((rotations/min)/s)
};

void Puller::setupPID() {
  this->pid = QuickPID(&input, &output, &setPoint, Kp, Ki, Kd, QuickPID::Action::direct);

  this->setPoint = diameter;
  this->minOutput = MIN_PULLER_SPEED_DEFAULT;
  this->maxOutput = MAX_PULLER_SPEED_DEFAULT;

  this->pid.SetTunings(this->Kp, this->Ki, this->Kd);
  this->pid.SetMode(QuickPID::Control::automatic);
}

void Puller::setupDriver() {
  this->puller.setupSpi(spi_parameters);

  this->puller.converter.setup(converter_parameters);

  tmc51x0::DriverParameters driver_parameters_chip = this->puller.converter.driverParametersRealToChip(driver_parameters_real);
  this->puller.driver.setup(driver_parameters_chip);

  tmc51x0::ControllerParameters controller_parameters_chip = this->puller.converter.controllerParametersRealToChip(controller_parameters_real);
  this->puller.controller.setup(controller_parameters_chip);

  this->puller.driver.enable();

  this->puller.controller.beginRampToZeroVelocity();
  while (!this->puller.controller.zeroVelocity());
  this->puller.controller.endRampToZeroVelocity();
  this->puller.controller.zeroActualPosition();
}

void Puller::setup() {
  this->setupPID();
  this->setupDriver();  

  this->minOutput = preferences.getUInt(MIN_PULLER_PREF, MIN_PULLER_SPEED_DEFAULT);
  this->maxOutput = preferences.getUInt(MAX_PULLER_PREF, MAX_PULLER_SPEED_DEFAULT);
}

void Puller::loop(unsigned long interval) {
  unsigned long currentMillis = millis();

  if (currentMillis - this->puller_loop_last_millis >= interval) {
    this->puller_loop_last_millis = currentMillis;

    this->input = measurement.lastRead;

    double gap = abs(this->setPoint - this->input);

    if (gap > PID_AGGRESSIVE_GAP) {
      this->pid.SetTunings(this->aggKp, this->aggKi, this->aggKd);
    } else {
      this->pid.SetTunings(this->Kp, this->Ki, this->Kd);
    }

    this->pid.Compute();

    this->speed = map(this->output, MIN_PID_PULLER_OUTPUT_LIMIT, MAX_PID_PULLER_OUTPUT_LIMIT, this->maxOutput, this->minOutput);

    this->puller.controller.writeMaxVelocity(this->puller.converter.velocityRealToChip(this->speed));

    if (this->puller.controller.readActualPosition() >= PULLER_ONE_REV_STEPS) {
      this->revs++;
      this->puller.controller.zeroActualPosition();
    }
  }
}

void Puller::updateSetPoint(float value) {
  this->setPoint = value;
}

void Puller::updateMinOutput(uint32_t value) {
  this->minOutput = value;

  preferences.writeUInt(MIN_PULLER_PREF, this->minOutput);
}

void Puller::updateMaxOutput(uint32_t value) {
  this->maxOutput = value;

  preferences.writeUInt(MAX_PULLER_PREF, this->maxOutput);
}