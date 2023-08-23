#pragma once

#include <Arduino.h>

class Task {
 public:
  bool initSummary;
  bool printStartPos;
  bool printEndPos;

  void initCoreZero();

  void initCoreOne();
};