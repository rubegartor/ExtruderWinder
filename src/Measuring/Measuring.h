#pragma once

#include <Arduino.h>

#define SERIAL2_TX 17
#define SERIAL2_RX 18

#define SERIAL2_RESPONSE_TIMEOUT 5000

#define RESET_MCU_PIN 42

#define AUTOSTOP_OUTPUT_PIN 47
#define AUTOSTOP_ENABLE_THRESHOLD 0.15

#define AUTOSTOP_THRESHOLD_PREF "autoStopThr"
#define AUTOSTOP_THRESHOLD_DEFAULT 0.2f

enum AutoStopStatus { autoStopEnabled, autoStopDisabled, autoStopTriggered };

class Measuring {
 private:
  long lastPingMillis;

  bool isValidMeasurement(String measurement);

  void restartMCU();

 public:
  AutoStopStatus autoStopStatus = autoStopDisabled;
  float readValueSum = 0.00f;
  uint32_t readValueNum = 0;
  float lastRead = 0.00f;
  float minRead = 0.00f;
  float maxRead = 0.00f;

  void init();

  float read();

  float average();

  void reset();

  void checkPingTimeout();
};