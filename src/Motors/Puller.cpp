#include "Puller.h"
#include "Commons/pins.h"
#include "Commons/globals.h"
#include "Stepper.h"

const auto spi_parameters = tmc51x0::SpiParameters{}
  .withSpi(&SPI_INTERFACE)
  .withChipSelectPin(PULLER_CS_PIN)
  .withClockRate(TMC5160_SPI_FREQ);

const auto converter_parameters = tmc51x0::ConverterParameters{}
  .withClockFrequencyMHz(12)
  .withMicrostepsPerRealPositionUnit(800)
  .withSecondsPerRealVelocityUnit(60);

const auto driver_parameters_real = tmc51x0::DriverParameters{}
  .withGlobalCurrentScaler(100)
  .withRunCurrent(30)
  .withHoldCurrent(20)
  .withHoldDelay(5)
  .withPwmOffset(30)
  .withPwmGradient(10)
  .withAutomaticCurrentControlEnabled(false)
  .withMotorDirection(tmc51x0::ReverseDirection)
  .withStandstillMode(tmc51x0::NormalMode)
  .withChopperMode(tmc51x0::SpreadCycleMode)
  .withStealthChopThreshold(60u)
  .withStealthChopEnabled(true)
  .withCoolStepThreshold(100u)
  .withCoolStepMin(1)
  .withCoolStepMax(0)
  .withCoolStepEnabled(false)
  .withHighVelocityThreshold(200u)
  .withHighVelocityFullstepEnabled(false)
  .withHighVelocityChopperSwitchEnabled(false)
  .withStallGuardThreshold(static_cast<int8_t>(0))
  .withStallGuardFilterEnabled(false)
  .withShortToGroundProtectionEnabled(false)
  .withEnabledToff(3)
  .withComparatorBlankTime(tmc51x0::ClockCycles36)
  .withDcTime(static_cast<uint16_t>(0))
  .withDcStallGuardThreshold(static_cast<uint8_t>(0));

const auto controller_parameters_real = tmc51x0::ControllerParameters{}
  .withRampMode(tmc51x0::VelocityPositiveMode)
  .withStopMode(tmc51x0::HardMode)
  .withMaxVelocity(0u)
  .withMaxAcceleration(1000000u);


void Puller::setup(unsigned long interval) {
  setInterval(interval);

  this->setupDriver();
  this->setupPID();

  this->minOutput = storage.getInt(MIN_PULLER_PREF, MIN_PULLER_SPEED_DEFAULT);
  this->maxOutput = storage.getInt(MAX_PULLER_PREF, MAX_PULLER_SPEED_DEFAULT);
}

void Puller::setupDriver() {
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

  motor.controller.writeMaxVelocity(motor.converter.velocityRealToChip(this->speed));
}

void Puller::applyParameters() {
  motor.driver.setup(motor.converter.driverParametersRealToChip(driver_parameters_real));
  motor.controller.setup(motor.converter.controllerParametersRealToChip(controller_parameters_real));
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
  TmcHealth health = checkTmcHealth(motor, guard, TMC_PULLER);
  if (health == TmcHealth::Lost) applyParameters();  // el chip se reinició: reescribir toda su config
  if (health != TmcHealth::Ok) return;

  this->input = measurementLastRead;

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
  int32_t positionDelta = (int32_t)((uint32_t)currentPosition - lastPosition);

  // Comprobación de cordura: al tope de la UI (18000) son 240 000 micropasos/s, unos
  // 19 200 por ciclo de 80 ms. Un delta negativo o de más de 2 vueltas es una lectura SPI
  // corrupta (o un reset del chip): resincronizar y descartar. (Antes se admitían 21 vueltas.)
  const int32_t MAX_POSITION_DELTA = PULLER_ONE_REV_STEPS * 2;
  if (positionDelta < 0 || positionDelta > MAX_POSITION_DELTA) {
    lastPosition = currentPosition;
    return;
  }

  int32_t completedRevolutions = positionDelta / PULLER_ONE_REV_STEPS;
  
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