#include <Arduino.h>
#include <Commons/Commons.h>
#include <Measuring/Measuring.h>

void Measuring::init() {
  Serial2.begin(115200, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);

  pinMode(AUTOSTOP_OUTPUT_PIN, OUTPUT);
  digitalWrite(AUTOSTOP_OUTPUT_PIN, LOW);

  pinMode(RESET_MCU_PIN, OUTPUT);
  digitalWrite(RESET_MCU_PIN, HIGH);

  this->mode =
      (MeasuringMode)pref.getUInt(MEASURING_MODE_PREF, measuringManualMode);
}

bool Measuring::isValidMeasurement(String measurement) {
  return measurement.indexOf('.') > 0 && measurement.length() == 4;
}

float Measuring::read() {
  String read;

  if (Serial2.available() > 0) {
    read = Serial2.readStringUntil('\r');
    read.trim();

    if (this->isValidMeasurement(read)) {
      float value = atof(read.c_str());

      // Si se detecta un cambio radical en comparación con el último valor
      // recibido se reinicia el MCU del medidor
      if (measuring.autoStopStatus == autoStopEnabled &&
          (millis() - millisOffset) > 1500 &&
          abs(this->lastRead - value) > 1.0f) {
#ifdef DEBUG
        Serial.println("[" + (String)millis() +
                       "] Restarting measuring MCU...");
#endif
        this->restartMCU();

        return this->lastRead;
      }

      if (millis() - this->lastSendOutMillis > 100 &&
          this->autoStopStatus != autoStopTriggered) {
        wifiOut.putEvent("lastRead", (String)this->lastRead);
        this->lastSendOutMillis = millis();
      }

      this->lastRead = value;
      this->readValueNum++;
      this->readValueSum += this->lastRead;
    }
  }

  if (this->autoStopStatus != autoStopTriggered) {
    if (this->lastRead < this->minRead) {
      this->minRead = this->lastRead;
      wifiOut.putEvent("minRead", (String)this->minRead);
    } else if (this->lastRead > this->maxRead) {
      this->maxRead = this->lastRead;
      wifiOut.putEvent("maxRead", (String)this->maxRead);
    }
  }

  if (this->mode == measuringAutoMode && this->autoStopStatus != autoStopTriggered) {
    float autoStopThr =
        pref.getFloat(AUTOSTOP_THRESHOLD_PREF, AUTOSTOP_THRESHOLD_DEFAULT);

    double gap = abs(pidPuller.getSetPoint() - this->lastRead);

    if (this->autoStopStatus == autoStopEnabled && gap > autoStopThr) {
      this->autoStopStatus = autoStopTriggered;

      // Detener el extrusor si se lanza el autoStop (activar optoacoplador)
      digitalWrite(AUTOSTOP_OUTPUT_PIN, HIGH);
      delay(250);
      digitalWrite(AUTOSTOP_OUTPUT_PIN, LOW);

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
  wifiOut.putEvent("reset", "");
}

void Measuring::restartMCU() {
  digitalWrite(RESET_MCU_PIN, LOW);
  delay(10);
  digitalWrite(RESET_MCU_PIN, HIGH);
}

String Measuring::measuringModeString() {
  String measuringModeString = "";

  if (this->mode == measuringAutoMode) {
    measuringModeString = "Auto  ";
  }

  if (this->mode == measuringManualMode) {
    measuringModeString = "Manual";
  }

  return measuringModeString;
}