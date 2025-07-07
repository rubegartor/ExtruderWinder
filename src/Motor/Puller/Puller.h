#pragma once

#include <TMC51X0.hpp>
#include <QuickPID.h>

#define PULLER_CS_PIN 10

#define MIN_PID_PULLER_OUTPUT_LIMIT 0
#define MAX_PID_PULLER_OUTPUT_LIMIT 255
#define PID_AGGRESSIVE_GAP 0.5f

#define MIN_PULLER_PREF "pMinSpeed"
#define MAX_PULLER_PREF "pMaxSpeed"
#define MIN_PULLER_SPEED_DEFAULT 1750
#define MAX_PULLER_SPEED_DEFAULT 8000

#define PULLER_DIAM 24.5f  // In mm
#define PULLER_DIAM_PERIMETER (PULLER_DIAM * PI)  // In mm

#define PULLER_MICROSTEPS 4

#define PULLER_ONE_REV_STEPS 51200

class Puller {
  private:
    TMC51X0 puller;
    QuickPID pid;

    float setPoint, input, output;
    float aggKp = 5, aggKi = 25, aggKd = 0.05;
    float Kp = 2, Ki = 12.5, Kd = 1;
    unsigned long puller_loop_last_millis = 0;

    void setupPID();
    void setupDriver();
  public:
    int minOutput;
    int maxOutput;
    uint16_t speed = 0;
    uint32_t revs = 0;
    uint32_t lastPosition = 0;

    void setup();
    void loop(unsigned long interval = 0);

    void updateSetPoint(float value);
    void updateMinOutput(uint32_t value);
    void updateMaxOutput(uint32_t value);
    bool enabled();
    bool drvErr();
    uint32_t drvStatusBytes();
    void reinit();
};