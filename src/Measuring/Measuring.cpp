#include <Arduino.h>
#include <Commons/Commons.h>
#include <Measuring/Measuring.h>

void Measuring::init() {
  this->mode =
      (MeasuringMode)pref.getUInt(MEASURING_MODE_PREF, measuringManualMode);
}

bool Measuring::isValidMeasurement(String measurement) {
  return measurement.indexOf('.') > 0 && measurement.length() == 4;
}

float Measuring::read() {
  String read;

  if (Serial2.available() > 0) {
    read = Serial2.readStringUntil('\n');
    read.trim();

    if (this->isValidMeasurement(read)) {
      this->lastRead = atof(read.c_str());
      this->readValueNum++;
      this->readValueSum += this->lastRead;

      if (millis() - this->lastSendOutMillis > 100) {
        wifiOut.putEvent("lastRead", (String)this->lastRead);
        this->lastSendOutMillis = millis();
      }
    }
  }

  if (this->lastRead < this->minRead) {
    this->minRead = this->lastRead;
    wifiOut.putEvent("minRead", (String)this->minRead);
  } else if (this->lastRead > this->maxRead) {
    this->maxRead = this->lastRead;
    wifiOut.putEvent("maxRead", (String)this->maxRead);
  }

  if (this->mode == measuringAutoMode) {
    float autoStopThr =
        pref.getFloat(AUTOSTOP_THRESHOLD_PREF, AUTOSTOP_THRESHOLD_DEFAULT);

    double gap = abs(pidPuller.getSetPoint() - this->lastRead);

    if (this->autoStopStatus == autoStopEnabled && gap > autoStopThr) {
      this->autoStopStatus = autoStopTriggered;

      Serial.println("Autostop triggered");
      Serial.println("String: " + read);
      Serial.println("Conversion: " + (String)this->lastRead);
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

  if (abs(gap) > AUTOSTOP_ENABLE_THRESHOLD) {
    this->autoStopStatus = autoStopEnabled;
  }

  this->minRead = this->lastRead;
  this->maxRead = this->lastRead;
  this->lastRead = 0;
  this->readValueNum = 0;
  this->readValueSum = 0;

  wifiOut.putEvent("minRead", (String)this->minRead);
  wifiOut.putEvent("maxRead", (String)this->minRead);
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