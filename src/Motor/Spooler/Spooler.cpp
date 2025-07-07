#include "Spooler.h"
#include "Commons/Commons.h"

const tmc51x0::SpiParameters spi_parameters =
{
  .spi_ptr = &SPI1,
  .clock_rate = 1000000,
  .chip_select_pin = SPOOLER_CS_PIN
};

const tmc51x0::ConverterParameters converter_parameters =
{
  .clock_frequency_mhz = TMC5160_CLK_FREQ,
  .microsteps_per_real_position_unit = 800,
};

const tmc51x0::DriverParameters driver_parameters_real =
{
  .global_current_scaler = 100,
  .run_current = 45,
  .hold_current = 45,
  .hold_delay = 0,
  .pwm_offset = 15,
  .pwm_gradient = 5,
  .automatic_current_control_enabled = false,
  .motor_direction = tmc51x0::ForwardDirection,
  .standstill_mode = tmc51x0::NormalMode,
  .chopper_mode = tmc51x0::SpreadCycleMode,
  .stealth_chop_threshold = 1,
  .stealth_chop_enabled = true,
  .cool_step_threshold = 1,
  .cool_step_min = 1,
  .cool_step_max = 0,
  .cool_step_enabled = true,
  .high_velocity_threshold = 20000,
  .high_velocity_fullstep_enabled = false,
  .high_velocity_chopper_switch_enabled = false,
  .stall_guard_threshold = 0,
  .stall_guard_filter_enabled = false,
  .short_to_ground_protection_enabled = true
};

const tmc51x0::ControllerParameters controller_parameters_real =
{
  .ramp_mode = tmc51x0::VelocityPositiveMode,
  .stop_mode = tmc51x0::SoftMode,
  .max_velocity = 0,
  .max_acceleration = SPOOLER_MAX_SPEED * 2,
};

void Spooler::setupPID() {
  this->pid = QuickPID(&input, &output, &setPoint, Kp, Ki, Kd, QuickPID::Action::direct);

  this->setPoint = TENSIONER_SETPOINT;
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

bool Spooler::enabled() {
  using namespace tmc51x0;

  tmc51x0::Registers::Chopconf chopConf;
  chopConf.bytes = this->spooler.registers.read(Registers::ChopconfAddress);

  return chopConf.toff != 0;
}

bool Spooler::drvErr() {
  using namespace tmc51x0;

  tmc51x0::Registers::Gstat gstat;
  gstat.bytes = this->spooler.registers.read(Registers::GstatAddress);

  return gstat.drv_err;
}

uint32_t Spooler::drvStatusBytes() {
  using namespace tmc51x0;

  tmc51x0::Registers::DrvStatus drv_status;
  drv_status.bytes = this->spooler.registers.read(Registers::DrvStatusAddress);

  return drv_status.bytes;
}

void Spooler::reinit() {
  digitalWrite(SPOOLER_CS_PIN, HIGH);

  this->spooler.reinitialize();
}