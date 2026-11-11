#pragma once

#include <TMC51X0.hpp>
#include "Commons/TimedComponent.h"
#include <QuickPID.h>
#include "Stepper.h"

#define SPOOLER_MAX_SPEED 65
#define MIN_PID_SPOOLER_OUTPUT_LIMIT 0
#define MAX_PID_SPOOLER_OUTPUT_LIMIT 255

#define SPOOLER_ONE_REV_STEPS 256000 // 51200 * 5, reducción 1:5

class Spooler : public TimedComponent {
private:
  TMC51X0 motor;
  QuickPID pid;
  
  uint32_t lastPosition;  // micropasos; unsigned para que el desbordamiento de XACTUAL (~2 h a máx.) no descuente vueltas
  int32_t revolutionCount;
  bool thresholdReached;
  TmcGuard guard;

  float setPoint, input, output;
  float Kp = 0.8, Ki = 0.5, Kd = 0.1;

  void setupPID();
  void setupDriver();
  void applyParameters();
    
public:
  uint16_t minOutput;
  uint16_t maxOutput;

  uint16_t speed;

  static Spooler& getInstance() {
    static Spooler instance;
    return instance;
  }

  Spooler(const Spooler&)            = delete;
  Spooler& operator=(const Spooler&) = delete;

  void setup(unsigned long interval);
  void execute() override;

  int32_t getRevolutionCount() const { return revolutionCount; }

private:
  Spooler() : TimedComponent() {}
};
