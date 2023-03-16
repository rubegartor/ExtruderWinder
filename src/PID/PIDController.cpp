#include <Commons/Commons.h>
#include <PID/PIDController.h>
#include <PID_v1.h>

// Define Variables we'll be connecting to
double Setpoint, Input, Output;

// Specify the links and initial tuning parameters
double Kp = 10, Ki = 25, Kd = 0.25;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

void PIDController::init() {
  Setpoint = filamentDiameter;
  myPID.SetMode(AUTOMATIC);
}

void PIDController::compute() {
  Input = calibration.lastRead;
  myPID.Compute();
  
  uint16_t computedValue = map(Output, 0, 255, 10000, 900);

  pullerSpeed = computedValue;

  Serial.println(computedValue);
}
