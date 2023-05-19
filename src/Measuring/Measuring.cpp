#include <Arduino.h>
#include <Commons/Commons.h>
#include <Measuring/Measuring.h>

void Measuring::init() {
  this->mode =
      (MeasuringMode)pref.getUInt(MEASURING_MODE_PREF, measuringManualMode);
}

float Measuring::read() {
  if (Serial2.available() > 0) {
    String read = Serial2.readStringUntil('\n');

    this->lastRead = atof(read.c_str());
    this->readValueNum++;
    this->readValueSum += this->lastRead;
  }

  if (this->lastRead < this->minRead) {
    this->minRead = this->lastRead;
  } else if (this->lastRead > this->maxRead) {
    this->maxRead = this->lastRead;
  }

  wifiOut.put("Extruder", "DiameterReading", (String)this->lastRead);

  if (this->mode == measuringAutoMode) {
    float autoStopThr =
        pref.getFloat(AUTOSTOP_THRESHOLD_PREF, AUTOSTOP_THRESHOLD_DEFAULT);

    double gap = abs(pidPuller.getSetPoint() - this->lastRead);

    if (this->autoStopEnabled && gap > autoStopThr) {
      pidPuller.emergencyStop();
    }
  }

  return this->lastRead;
}

float Measuring::average() {
  if (this->readValueNum == 0) return 0.00f;

  return ((this->readValueSum / this->readValueNum) * 100) / 100;
}

void Measuring::reset() {
  // Si el margen de error entre el setPoint y la lectura actual es mayor a
  // AUTOSTOP_ENABLE_THRESHOLD se activa el autoStop
  double gap =
      abs(pidPuller.getSetPoint() - this->lastRead) - pidPuller.getSetPoint();

  this->autoStopEnabled = abs(gap) > AUTOSTOP_ENABLE_THRESHOLD;

  this->minRead = this->lastRead;
  this->maxRead = this->lastRead;
  this->lastRead = 0;
  this->readValueNum = 0;
  this->readValueSum = 0;

  wifiOut.put("Extruder", "Reset");
}

String Measuring::measuringModeString() {
  String measuringModeString = "";

  if (measuring.mode == measuringAutoMode) {
    measuringModeString = "Auto  ";
  }

  if (measuring.mode == measuringManualMode) {
    measuringModeString = "Manual";
  }

  return measuringModeString;
}