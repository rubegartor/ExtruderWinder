#include "Puller.h"
#include "Commons/Commons.h"

const tmc51x0::SpiParameters spi_parameters =
{
  .spi_ptr = &SPI1,
  .clock_rate = 1000000,
  .chip_select_pin = PULLER_CS_PIN
};

const tmc51x0::ConverterParameters converter_parameters =
{
  .clock_frequency_mhz = TMC5160_CLK_FREQ,
  .microsteps_per_real_position_unit = STEPPER_DEFAULT_STEPS * PULLER_MICROSTEPS,
  .seconds_per_real_velocity_unit = 60
};

const tmc51x0::DriverParameters driver_parameters_real =
{
  .global_current_scaler = 100,
  .run_current = 25,
  .hold_current = 45,
  .hold_delay = 0,
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
  .cool_step_enabled = true,
  .high_velocity_threshold = 1,
  .high_velocity_fullstep_enabled = false,
  .high_velocity_chopper_switch_enabled = false,
  .stall_guard_threshold = 0,
  .stall_guard_filter_enabled = false,
  .short_to_ground_protection_enabled = true
};

const tmc51x0::ControllerParameters controller_parameters_real =
{
  .ramp_mode = tmc51x0::VelocityPositiveMode,
  .stop_mode = tmc51x0::HardMode,
  .max_velocity = 0,
  .max_acceleration = 1000000,
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

  this->minOutput = storage.getInt(MIN_PULLER_PREF, MIN_PULLER_SPEED_DEFAULT);
  this->maxOutput = storage.getInt(MAX_PULLER_PREF, MAX_PULLER_SPEED_DEFAULT);
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

  storage.setInt(MIN_PULLER_PREF, this->minOutput);
}

void Puller::updateMaxOutput(uint32_t value) {
  this->maxOutput = value;

  storage.setInt(MAX_PULLER_PREF, this->maxOutput);
}

bool Puller::enabled() {
  using namespace tmc51x0;

  tmc51x0::Registers::Chopconf chopConf;
  chopConf.bytes = this->puller.registers.read(Registers::ChopconfAddress);

  return chopConf.toff != 0;
}

bool Puller::drvErr() {
  using namespace tmc51x0;

  tmc51x0::Registers::Gstat gstat;
  gstat.bytes = this->puller.registers.read(Registers::GstatAddress);

  return gstat.drv_err;
}

uint32_t Puller::drvStatusBytes() {
  using namespace tmc51x0;

  tmc51x0::Registers::DrvStatus drv_status;
  drv_status.bytes = this->puller.registers.read(Registers::DrvStatusAddress);

  return drv_status.bytes;
}

void Puller::reinit() {
  digitalWrite(PULLER_CS_PIN, HIGH);

  this->puller.reinitialize();
}