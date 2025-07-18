#include "Puller.h"
#include "Commons/pins.h"
#include "Commons/globals.h"

const tmc51x0::SpiParameters spi_parameters =
{
  .spi_ptr = &SPI_INTERFACE,
  .clock_rate = 2000000,
  .chip_select_pin = PULLER_CS_PIN
};

const tmc51x0::ConverterParameters converter_parameters =
{
  .clock_frequency_mhz = 12,
  .microsteps_per_real_position_unit = 800,
  .seconds_per_real_velocity_unit = 60
};

const tmc51x0::DriverParameters driver_parameters_real =
{
  .global_current_scaler = 100,
  .run_current = 25,
  .hold_current = 20,
  .hold_delay = 5,
  .pwm_offset = 30,
  .pwm_gradient = 10,
  .automatic_current_control_enabled = false,
  .motor_direction = tmc51x0::ReverseDirection,
  .standstill_mode = tmc51x0::NormalMode,
  .chopper_mode = tmc51x0::SpreadCycleMode,
  .stealth_chop_threshold = 60,
  .stealth_chop_enabled = true,
  .cool_step_threshold = 100,
  .cool_step_min = 1,
  .cool_step_max = 0,
  .cool_step_enabled = false,
  .high_velocity_threshold = 200,
  .high_velocity_fullstep_enabled = false,
  .high_velocity_chopper_switch_enabled = false,
  .stall_guard_threshold = 0,
  .stall_guard_filter_enabled = false,
  .short_to_ground_protection_enabled = false,
  .enabled_toff = 3,
  .comparator_blank_time = tmc51x0::ClockCycles36,
  .dc_time = 0,
  .dc_stall_guard_threshold = 0
};

const tmc51x0::ControllerParameters controller_parameters_real =
{
  .ramp_mode = tmc51x0::VelocityPositiveMode,
  .stop_mode = tmc51x0::HardMode,
  .max_velocity = 0,
  .max_acceleration = 1000000
};

void Puller::setup() {
  this->setupDriver();
  this->setupPID();

  this->minOutput = storage.getInt(MIN_PULLER_PREF, MIN_PULLER_SPEED_DEFAULT);
  this->maxOutput = storage.getInt(MAX_PULLER_PREF, MAX_PULLER_SPEED_DEFAULT);
}

void Puller::setupDriver() {
  motor.setupSpi(spi_parameters);
  motor.converter.setup(converter_parameters);

  tmc51x0::DriverParameters driver_parameters_chip = motor.converter.driverParametersRealToChip(driver_parameters_real);
  motor.driver.setup(driver_parameters_chip);

  tmc51x0::ControllerParameters controller_parameters_chip = motor.converter.controllerParametersRealToChip(controller_parameters_real);
  motor.controller.setup(controller_parameters_chip);

  motor.driver.enable();

  motor.controller.beginRampToZeroVelocity();
  while (!motor.controller.zeroVelocity());
  motor.controller.endRampToZeroVelocity();
  motor.controller.zeroActualPosition();

  lastPosition = 0;
  revolutionCount = 0;

  motor.controller.writeMaxVelocity(motor.converter.velocityRealToChip(this->speed));
}

void Puller::setupPID() {
  this->pid = QuickPID(&input, &output, &setPoint, Kp, Ki, Kd, QuickPID::Action::direct);

  this->setPoint = diameter;
  this->minOutput = MIN_PULLER_SPEED_DEFAULT;
  this->maxOutput = MAX_PULLER_SPEED_DEFAULT;

  this->pid.SetTunings(this->Kp, this->Ki, this->Kd);
  this->pid.SetMode(QuickPID::Control::automatic);
}

void Puller::execute() {
  this->input = measurement.lastRead;

  double gap = abs(this->setPoint - this->input);

  if (gap > PID_AGGRESSIVE_GAP) {
    this->pid.SetTunings(this->aggKp, this->aggKi, this->aggKd);
  } else {
    this->pid.SetTunings(this->Kp, this->Ki, this->Kd);
  }

  this->pid.Compute();

  this->speed = map(this->output, MIN_PID_PULLER_OUTPUT_LIMIT, MAX_PID_PULLER_OUTPUT_LIMIT, this->maxOutput, this->minOutput);

  motor.controller.writeMaxVelocity(motor.converter.velocityRealToChip(this->speed));

  int32_t currentPosition = motor.controller.readActualPosition();
  int32_t completedRevolutions = (currentPosition - lastPosition) / PULLER_ONE_REV_STEPS;
  
  if (completedRevolutions >= 1) {
    revolutionCount += completedRevolutions;
    lastPosition += completedRevolutions * PULLER_ONE_REV_STEPS;
  }
}

void Puller::updateSetPoint(float value) {
  this->setPoint = value;
}

void Puller::updateMinOutput(uint32_t value) {
  this->minOutput = value;

  storage.setInt(MIN_PULLER_PREF, this->minOutput);
}

void Puller::updateMaxOutput(uint32_t value) {
  this->maxOutput = value;

  storage.setInt(MAX_PULLER_PREF, this->maxOutput);
}