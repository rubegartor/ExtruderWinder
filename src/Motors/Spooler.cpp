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
  .withMicrostepsPerRealPositionUnit(SPOOLER_ONE_REV_STEPS)
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

void Spooler::setup(unsigned long interval) {
  setInterval(interval);

  setupPID();
  setupDriver();
}

void Spooler::setupDriver() {
  motor.setupSpi(spi_parameters, tmc51x0::Registers::DeviceModel::TMC5160A);
  motor.converter.setup(converter_parameters);

  applyParameters();

  motor.driver.enable();

  motor.controller.beginRampToZeroVelocity();
  unsigned long zeroVelTimeout = millis() + 5000;
  while (!motor.controller.zeroVelocity() && millis() < zeroVelTimeout);
  motor.controller.endRampToZeroVelocity();
  motor.controller.zeroActualPosition();
  // endRampToZeroVelocity() restaura el modo de rampa que LEYÓ del chip: fijar el nuestro.
  applyParameters();

  lastPosition = 0;
  revolutionCount = 0;
  thresholdReached = false;
}

void Spooler::applyParameters() {
  motor.driver.setup(motor.converter.driverParametersRealToChip(driver_parameters_real));
  motor.controller.setup(motor.converter.controllerParametersRealToChip(controller_parameters_real));
}

void Spooler::setupPID() {
  this->pid = QuickPID(&input, &output, &setPoint, Kp, Ki, Kd, QuickPID::Action::direct);

  this->setPoint = TENSIONER_SETPOINT;
  this->minOutput = 0;
  this->maxOutput = SPOOLER_MAX_SPEED;

  this->pid.SetMode(QuickPID::Control::automatic);
}

void Spooler::execute() {
  TmcHealth health = checkTmcHealth(motor, guard, TMC_SPOOLER);
  if (health == TmcHealth::Lost) applyParameters();  // el chip se reinició: reescribir toda su config
  if (health != TmcHealth::Ok) return;

  if (!aligner.isSpoolCalibrated()) {
    motor.controller.writeMaxVelocity(0);
    lastPosition = motor.controller.readActualPosition();
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
    lastPosition = motor.controller.readActualPosition();
    return;
  }

  if (tensioner.distance <= TENSIONER_MIN_HARD_LIMIT) {
    motor.controller.writeMaxAcceleration(motor.converter.accelerationRealToChip(SPOOLER_MAX_SPEED * 2));
    motor.controller.writeMaxVelocity(0);
    lastPosition = motor.controller.readActualPosition();
    return;
  } else {
    motor.controller.writeMaxAcceleration(motor.converter.accelerationRealToChip(10));
    this->pid.Compute();
    this->speed = map(this->output, MIN_PID_SPOOLER_OUTPUT_LIMIT, MAX_PID_SPOOLER_OUTPUT_LIMIT, this->maxOutput, this->minOutput);
  }

  motor.controller.writeMaxVelocity(motor.converter.velocityRealToChip(this->speed));

  int32_t currentPosition = motor.controller.readActualPosition();
  int32_t positionDelta = (int32_t)((uint32_t)currentPosition - lastPosition);

  // Comprobación de cordura: a SPOOLER_MAX_SPEED RPM se avanzan ~0.1 vueltas por
  // ciclo, así que lastPosition nunca queda a más de ~1.1 vueltas. Un delta mayor
  // o negativo es una lectura SPI corrupta (o un reinit): resincronizar y descartar.
  // (Antes se admitían hasta 5 vueltas: un bit erróneo movía el aligner 5 pasos.)
  if (positionDelta < 0 || positionDelta > SPOOLER_ONE_REV_STEPS * 3 / 2) {
    lastPosition = currentPosition;
    return;
  }

  int32_t completedRevolutions = positionDelta / SPOOLER_ONE_REV_STEPS;

  if (completedRevolutions >= 1) {
    revolutionCount += completedRevolutions;
    lastPosition += completedRevolutions * SPOOLER_ONE_REV_STEPS;
    
    for (int i = 0; i < completedRevolutions; i++) {
      aligner.onSpoolerRevolution();
    }
  }
}
