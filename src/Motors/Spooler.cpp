#include "Spooler.h"
#include "Commons/pins.h"
#include "Commons/globals.h"
#include "Stepper.h"

const tmc51x0::SpiParameters spi_parameters =
{
  .spi_ptr = &SPI_INTERFACE,
  .clock_rate = TMC5160_SPI_FREQ,
  .chip_select_pin = SPOOLER_CS_PIN
};

const tmc51x0::ConverterParameters converter_parameters =
{
  .clock_frequency_mhz = 12,
  .microsteps_per_real_position_unit = 256000,  // 51200 * 5 (reducción 1:5)
  .seconds_per_real_velocity_unit = 60
};

const tmc51x0::DriverParameters driver_parameters_real =
{
  .global_current_scaler = 100,
  .run_current = 85,
  .hold_current = 25,
  .hold_delay = 5,
  .pwm_offset = 30,
  .pwm_gradient = 10,
  .automatic_current_control_enabled = false,
  .motor_direction = tmc51x0::ForwardDirection,
  .standstill_mode = tmc51x0::NormalMode,
  .chopper_mode = tmc51x0::SpreadCycleMode,
  .stealth_chop_threshold = 0,
  .stealth_chop_enabled = false,
  .cool_step_threshold = 1000,
  .cool_step_min = 1,
  .cool_step_max = 0,
  .cool_step_enabled = false,
  .high_velocity_threshold = 0,
  .high_velocity_fullstep_enabled = false,
  .high_velocity_chopper_switch_enabled = false,
  .stall_guard_threshold = 0,
  .stall_guard_filter_enabled = false,
  .short_to_ground_protection_enabled = true,
  .enabled_toff = 5,
  .comparator_blank_time = tmc51x0::ClockCycles36,
  .dc_time = 0,
  .dc_stall_guard_threshold = 0,
};

const tmc51x0::ControllerParameters controller_parameters_real =
{
  .ramp_mode = tmc51x0::VelocityPositiveMode,
  .stop_mode = tmc51x0::HardMode,
  .max_velocity = 0,
  .max_acceleration = 10
};

void Spooler::setup() {
  setupPID();
  setupDriver();
}

void Spooler::setupDriver() {
  motor.setupSpi(spi_parameters);
  motor.converter.setup(converter_parameters);

  tmc51x0::DriverParameters driver_parameters_chip = motor.converter.driverParametersRealToChip(driver_parameters_real);
  motor.driver.setup(driver_parameters_chip);

  tmc51x0::ControllerParameters controller_parameters_chip = motor.converter.controllerParametersRealToChip(controller_parameters_real);
  motor.controller.setup(controller_parameters_chip);

  motor.driver.enable();

  motor.controller.beginRampToZeroVelocity();
  unsigned long zeroVelTimeout = millis() + 5000;
  while (!motor.controller.zeroVelocity() && millis() < zeroVelTimeout);
  motor.controller.endRampToZeroVelocity();
  motor.controller.zeroActualPosition();
  

  lastPosition = 0;
  revolutionCount = 0;
  thresholdReached = false;
}

void Spooler::setupPID() {
  this->pid = QuickPID(&input, &output, &setPoint, Kp, Ki, Kd, QuickPID::Action::direct);

  this->setPoint = TENSIONER_SETPOINT;
  this->minOutput = 0;
  this->maxOutput = SPOOLER_MAX_SPEED;

  this->pid.SetMode(QuickPID::Control::automatic);
}

void Spooler::execute() {
  if (!this->enabled()) {
    this->reinit();
  }

  if (!aligner.isSpoolCalibrated()) {
    motor.controller.writeMaxVelocity(0);
    lastPosition = motor.converter.positionChipToReal(motor.controller.readActualPosition());
    revolutionCount = 0;
    thresholdReached = false;
    return;
  }

  this->input = float(tensioner.distance);

  if (this->input >= TENSIONER_START_THRESHOLD) {
    thresholdReached = true;
  }

  if (!thresholdReached && revolutionCount == 0 && this->input < TENSIONER_START_THRESHOLD) {
    motor.controller.writeMaxVelocity(0);
    lastPosition = motor.converter.positionChipToReal(motor.controller.readActualPosition());
    return;
  }

  if (tensioner.distance <= TENSIONER_MIN_HARD_LIMIT) {
    motor.controller.writeMaxAcceleration(motor.converter.accelerationRealToChip(SPOOLER_MAX_SPEED * 2));
    motor.controller.writeMaxVelocity(0);
    lastPosition = motor.converter.positionChipToReal(motor.controller.readActualPosition());
    return;
  } else {
    motor.controller.writeMaxAcceleration(motor.converter.accelerationRealToChip(10));
    this->pid.Compute();
    this->speed = map(this->output, MIN_PID_SPOOLER_OUTPUT_LIMIT, MAX_PID_SPOOLER_OUTPUT_LIMIT, this->maxOutput, this->minOutput);
  }

  motor.controller.writeMaxVelocity(motor.converter.velocityRealToChip(this->speed));

  int32_t currentPosition = motor.converter.positionChipToReal(motor.controller.readActualPosition());
  int32_t completedRevolutions = currentPosition - lastPosition;

  // Comprobación de cordura: en el intervalo de ejecución (100 ms) a
  // SPOOLER_MAX_SPEED RPM el máximo posible es ~1 revolución. Si es mayor,
  // la lectura SPI está corrompida; actualizar lastPosition y descartar.
  if (completedRevolutions < 0 || completedRevolutions > 5) {
    lastPosition = currentPosition;
    return;
  }
  
  if (completedRevolutions >= 1) {
    revolutionCount += completedRevolutions;
    lastPosition = currentPosition;
    
    for (int i = 0; i < completedRevolutions; i++) {
      aligner.onSpoolerRevolution();
    }
  }
}

void Spooler::reinit() {
  // No llamar a SPI1.end()/begin(): mataría la comunicación del Puller y Aligner también.
  // Solo reinicializar este driver TMC manteniendo el bus SPI activo.
  motor.reinitialize();
  delay(10);

  tmc51x0::DriverParameters driver_parameters_chip = motor.converter.driverParametersRealToChip(driver_parameters_real);
  motor.driver.setup(driver_parameters_chip);

  tmc51x0::ControllerParameters controller_parameters_chip = motor.converter.controllerParametersRealToChip(controller_parameters_real);
  motor.controller.setup(controller_parameters_chip);

  motor.driver.enable();
  disabledCount = 0;
}

bool Spooler::enabled() {
  using namespace tmc51x0;

  tmc51x0::Registers::Chopconf chopConf;
  chopConf.bytes = motor.registers.read(Registers::ChopconfAddress);

  if (chopConf.toff != 0) {
    disabledCount = 0;
    return true;
  }

  // Requerir fallos consecutivos antes de considerar el driver deshabilitado.
  // Un solo fallo SPI por ruido no debe lanzar un reinit en producción.
  disabledCount++;
  return disabledCount < SPOOLER_ENABLED_FAIL_THRESHOLD;
}