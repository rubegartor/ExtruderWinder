#pragma once

#include <TMC51X0.hpp>
#include "Commons/TimedComponent.h"
#include "Stepper.h"

#define STEPS_PER_CM 52350
#define MAX_ALIGNER_POSITION (STEPS_PER_CM * 13)
#define MANUAL_MOVE_DELAY 25

enum AlignerDirection {
  FORWARD,
  BACKWARD
};

enum AlignerParameterMode {
  NORMAL_OPERATION,
  CALIBRATION
};

enum AlignerState {
  HOMING,            // Buscando el límite inicial
  MOVING_TO_HOME,    // Movimiento hacia el home
  SPOOL_CALIBRATION, // Calibración del spool
  AUTO_MOVE,         // Movimiento automático
  IDLE,              // Estado inactivo
  WAITING            // Estado de espera no bloqueante
};

class Aligner : public TimedComponent {
private:
  TMC51X0 motor;
  AlignerState currentState = AlignerState::HOMING;
  AlignerDirection direction = FORWARD;

  unsigned long aligner_manual_movement_last_millis = 0;

  // Variables para el estado de espera genérico
  unsigned long waitStartTime = 0;
  unsigned long waitDuration = 0;
  AlignerState nextStateAfterWait = AlignerState::IDLE;

  bool homingAfterError = false;
  int32_t lastKnownPosition = 0;
  int32_t targetPosition = 0;   // Copia en RAM de XTARGET (ver writeTarget)
  TmcGuard guard;

  // Acciones diferidas desde la UI, ejecutadas en el motorTask (ver processPendingActions).
  volatile bool    pendingResetHome = false;
  volatile bool    pendingStartCalibration = false;
  volatile bool    pendingManualMove = false;
  volatile int32_t pendingManualMoveDelta = 0;

  // Variables para el estado de calibración del spool
  bool startPositionSet = false;
  bool endPositionSet = false;
  
  // Variables para extensiones de posición.
  // volatile: se escriben desde callbacks de la UI y se leen en el motorTask
  // (getEffectiveStartPos/EndPos durante onSpoolerRevolution).
  volatile int32_t startExtension = 0;  // Extensión hacia la izquierda desde startPos
  volatile int32_t endExtension = 0;    // Extensión hacia la derecha desde endPos

public:
  volatile bool canMoveLeft = false;
  volatile bool canMoveRight = false;

  // Acciones solicitadas desde el hilo de la UI. NO ejecutan SPI directamente:
  // marcan un flag que el motorTask procesa en processPendingActions(), de modo
  // que TODO acceso al bus SPI ocurre en un único hilo y nunca colisiona.
  void requestManualMove(int32_t delta);
  void requestResetHome();
  void requestStartCalibration();

  int32_t startPos = 0;
  int32_t endPos = MAX_ALIGNER_POSITION;

  static Aligner& getInstance() {
    static Aligner instance;
    return instance;
  }

  Aligner(const Aligner&)            = delete;
  Aligner& operator=(const Aligner&) = delete;

  void setup(unsigned long interval);
  void execute() override;
  void loop() override;

  void onSpoolerRevolution();

  void moveTo(int32_t position);

  bool isSpoolCalibrated();
  bool isHoming();
  int32_t currentPosition();

  void resetHome(bool afterError = false);
  void startSpoolCalibration();
  
  void addStartExtension();
  void removeStartExtension();
  void addEndExtension();
  void removeEndExtension();

  int32_t getStartExtensionSteps();
  int32_t getEndExtensionSteps();
  int32_t getEffectiveStartPos();
  int32_t getEffectiveEndPos();
  
  AlignerState getCurrentState() const { return currentState; }
  AlignerDirection getCurrentDirection() const { return direction; }
  int32_t getStartExtension() const { return startExtension; }
  int32_t getEndExtension() const { return endExtension; }
  
  void waitFor(unsigned long durationMs, AlignerState nextState);

private:
  void reinit();
  Aligner() : TimedComponent() {}

  void applyParameters(AlignerParameterMode mode);
  void resetPositions();
  void setStartPosition();
  void setEndPosition();
  void handleStateMachine();
  void handleHoming();
  void handleMovingToHome();
  void handleSpoolCalibration();
  void handleWaiting();
  void handleManualMovement();
  void processPendingActions();
  void writeTarget(int32_t position);
  int32_t readStoppedPosition();
  bool stalled();
  bool positionReached();
};
