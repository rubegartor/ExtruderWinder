#include "Spooler.h"
#include "Commons/pins.h"
#include "Commons/globals.h"

const tmc51x0::SpiParameters spi_parameters =
{
  .spi_ptr = &SPI_INTERFACE,
  .clock_rate = 2000000,
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
  .run_current = 55,
  .hold_current = 35,
  .hold_delay = 2,
  .pwm_offset = 45,
  .pwm_gradient = 25,
  .automatic_current_control_enabled = false,
  .motor_direction = tmc51x0::ForwardDirection,
  .standstill_mode = tmc51x0::NormalMode,
  .chopper_mode = tmc51x0::SpreadCycleMode,
  .stealth_chop_threshold = 30,
  .stealth_chop_enabled = true,
  .cool_step_threshold = 30,
  .cool_step_min = 1,
  .cool_step_max = 10,
  .cool_step_enabled = true,
  .high_velocity_threshold = 30,
  .high_velocity_fullstep_enabled = false,
  .high_velocity_chopper_switch_enabled = false,
  .stall_guard_threshold = 0,
  .stall_guard_filter_enabled = false,
  .short_to_ground_protection_enabled = true,
  .enabled_toff = 5,
  .comparator_blank_time = tmc51x0::ClockCycles16,
  .dc_time = 0,
  .dc_stall_guard_threshold = 0,
};

const tmc51x0::ControllerParameters controller_parameters_real =
{
  .ramp_mode = tmc51x0::VelocityPositiveMode,
  .stop_mode = tmc51x0::HardMode,
  .max_velocity = 0,
  .max_acceleration = 10,
  .start_velocity = 1,
  .stop_velocity = 10,
  .first_velocity = 0,
  .first_acceleration = 0,
  .max_deceleration = 0,
  .first_deceleration = 10,
  .zero_wait_duration = 0,
  .stall_stop_enabled = false,
  .min_dc_step_velocity = 0
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
  while (!motor.controller.zeroVelocity());
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
  if (!aligner.isSpoolCalibrated()) {
    motor.controller.writeMaxVelocity(0);
    // Reiniciar el conteo cuando no está calibrado
    lastPosition = motor.converter.positionChipToReal(motor.controller.readActualPosition());
    revolutionCount = 0;
    thresholdReached = false;
    return;
  }

  this->input = float(tensioner.distance);

  // Verificar si se ha alcanzado el threshold
  if (this->input >= TENSIONER_START_THRESHOLD) {
    thresholdReached = true;
  }

  // Si no se ha alcanzado el threshold y es la primera revolución, mantener velocidad 0
  if (!thresholdReached && revolutionCount == 0 && this->input < TENSIONER_START_THRESHOLD) {
    motor.controller.writeMaxVelocity(0);
    // Actualizar la posición base para el conteo de revoluciones
    lastPosition = motor.converter.positionChipToReal(motor.controller.readActualPosition());
    return;
  }

  if (this->input <= TENSIONER_MIN_HARD_LIMIT) {
    this->speed = 0;
  } else {
    this->pid.Compute();

    this->speed = map(this->output, MIN_PID_SPOOLER_OUTPUT_LIMIT, MAX_PID_SPOOLER_OUTPUT_LIMIT, this->maxOutput, this->minOutput);
  }

  motor.controller.writeMaxVelocity(motor.converter.velocityRealToChip(this->speed));

  int32_t currentPosition = motor.converter.positionChipToReal(motor.controller.readActualPosition());
  int32_t completedRevolutions = currentPosition - lastPosition;
  
  if (completedRevolutions >= 1) {
    revolutionCount += completedRevolutions;
    lastPosition = currentPosition;
    
    // Llamar al aligner por cada revolución completada
    for (int i = 0; i < completedRevolutions; i++) {
      aligner.onSpoolerRevolution();
    }
  }
}