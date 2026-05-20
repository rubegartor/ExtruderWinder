#include "Spooler.h"
#include "Commons/pins.h"
#include "Commons/globals.h"
#include "Stepper.h"

const auto spi_parameters = tmc51x0::SpiParameters{}
  .withSpi(&SPI_INTERFACE)
  .withChipSelectPin(SPOOLER_CS_PIN)
  .withClockRate(TMC5160_SPI_FREQ);

const auto converter_parameters = tmc51x0::ConverterParameters{}
  .withClockFrequencyMHz(12)
  .withMicrostepsPerRealPositionUnit(256000) // (51200 * 5, reducción 1:5)
  .withSecondsPerRealVelocityUnit(60);

const auto driver_parameters_real = tmc51x0::DriverParameters{}
  .withGlobalCurrentScaler(100)
  .withRunCurrent(85)
  .withHoldCurrent(25)
  .withHoldDelay(5)
  .withPwmOffset(30)
  .withPwmGradient(10)
  .withAutomaticCurrentControlEnabled(false)
  .withMotorDirection(tmc51x0::ForwardDirection)
  .withStandstillMode(tmc51x0::NormalMode)
  .withChopperMode(tmc51x0::SpreadCycleMode)
  .withStealthChopThreshold(0u)
  .withStealthChopEnabled(false)
  .withCoolStepThreshold(1000u)
  .withCoolStepMin(1)
  .withCoolStepMax(0)
  .withCoolStepEnabled(false)
  .withHighVelocityThreshold(0u)
  .withHighVelocityFullstepEnabled(false)
  .withHighVelocityChopperSwitchEnabled(false)
  .withStallGuardThreshold(static_cast<int8_t>(0))
  .withStallGuardFilterEnabled(false)
  .withShortToGroundProtectionEnabled(true)
  .withEnabledToff(5)
  .withComparatorBlankTime(tmc51x0::ClockCycles36)
  .withDcTime(static_cast<uint16_t>(0))
  .withDcStallGuardThreshold(static_cast<uint8_t>(0));

const auto controller_parameters_real = tmc51x0::ControllerParameters{}
  .withRampMode(tmc51x0::VelocityPositiveMode)
  .withStopMode(tmc51x0::HardMode)
  .withMaxVelocity(0u)
  .withMaxAcceleration(10u);

void Spooler::setup() {
  setupPID();
  setupDriver();
}

void Spooler::setupDriver() {
  motor.setupSpi(spi_parameters, tmc51x0::Registers::DeviceModel::TMC5160A);
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
  motor.recoverIfUnhealthy();

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
  chopConf.raw = motor.registers.read(Registers::ChopconfAddress);

  if (chopConf.toff() != 0) {
    disabledCount = 0;
    return true;
  }

  // Requerir fallos consecutivos antes de considerar el driver deshabilitado.
  // Un solo fallo SPI por ruido no debe lanzar un reinit en producción.
  disabledCount++;
  return disabledCount < SPOOLER_ENABLED_FAIL_THRESHOLD;
}