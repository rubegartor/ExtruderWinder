#include <Arduino.h>
#include <Commons/Commons.h>
#include <Measuring/Measuring.h>

void Measuring::init() {
  Serial2.begin(115200, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);

  pinMode(AUTOSTOP_OUTPUT_PIN, OUTPUT);
  digitalWrite(AUTOSTOP_OUTPUT_PIN, LOW);

  pinMode(RESET_MCU_PIN, OUTPUT);
  digitalWrite(RESET_MCU_PIN, HIGH);
}

bool Measuring::isValidMeasurement(String measurement) {
  return measurement.indexOf('.') > 0 && measurement.length() == 4;
}

float Measuring::read() {
  String read;

  while (Serial2.available()) {
    char b = (char)Serial2.read();

    if (b == '\n') break;

    read += b;
  }

  read.trim();

  if (!read.isEmpty()) {
    if (read.charAt(0) == 'P') {
      this->lastPingMillis = millis();
    }

    if (this->isValidMeasurement(read)) {
      float value = atof(read.c_str());

      // Si se detecta un cambio radical en comparación con el último valor
      // recibido se reinicia el MCU del medidor
      if (measuring.autoStopStatus == autoStopEnabled &&
          (millis() - millisOffset) > 1500 &&
          abs(this->lastRead - value) > 1.0f) {
        this->restartMCU();

        return this->lastRead;
      }

      communication.sendEvent("lastRead", (String)this->lastRead);

      this->lastRead = value;
      this->readValueNum++;
      this->readValueSum += this->lastRead;
    }

    if (this->autoStopStatus != autoStopTriggered) {
      if (this->lastRead < this->minRead) {
        this->minRead = this->lastRead;
        communication.sendEvent("minRead", (String)this->minRead);
      } else if (this->lastRead > this->maxRead) {
        this->maxRead = this->lastRead;
        communication.sendEvent("maxRead", (String)this->maxRead);
      }
    }

    if (this->autoStopStatus != autoStopTriggered) {
      float autoStopThr =
          pref.getFloat(AUTOSTOP_THRESHOLD_PREF, AUTOSTOP_THRESHOLD_DEFAULT);

      double gap = abs(pidPuller.getSetPoint() - this->lastRead);

      if (this->autoStopStatus == autoStopEnabled && gap > autoStopThr) {
        this->autoStopStatus = autoStopTriggered;
        communication.sendEvent("stAutoStop", (String)this->autoStopStatus);

        // Detener el extrusor si se lanza el autoStop (activar optoacoplador)
        digitalWrite(AUTOSTOP_OUTPUT_PIN, HIGH);
        delay(250);
        digitalWrite(AUTOSTOP_OUTPUT_PIN, LOW);

        Serial.println("Autostop triggered");
        Serial.println("String: " + read);
        Serial.println("Conversion: " + (String)this->lastRead);
      }
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
  if (this->lastRead >= pidPuller.getSetPoint() - AUTOSTOP_ENABLE_THRESHOLD && this->lastRead <= pidPuller.getSetPoint() + AUTOSTOP_ENABLE_THRESHOLD) {
    double gap =
        abs(pidPuller.getSetPoint() - this->lastRead) - pidPuller.getSetPoint();

    if ((abs(gap) - pidPuller.getSetPoint()) <= AUTOSTOP_ENABLE_THRESHOLD) {
      this->autoStopStatus = autoStopEnabled;
      communication.sendEvent("stAutoStop", (String)this->autoStopStatus);
    }
  }

  this->minRead = this->lastRead;
  this->maxRead = this->lastRead;
  this->lastRead = 0;
  this->readValueNum = 0;
  this->readValueSum = 0;

  communication.sendEvent("minRead", (String)this->minRead);
  communication.sendEvent("maxRead", (String)this->minRead);
}

void Measuring::restartMCU() {
#ifdef DEBUG
  Serial.println("[" + (String)millis() +
                  "] Restarting measuring MCU...");
#endif

  digitalWrite(RESET_MCU_PIN, LOW);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(10);
  digitalWrite(RESET_MCU_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  this->lastPingMillis = millis();
}

void Measuring::checkPingTimeout() {
  if (millis() - this->lastPingMillis >= SERIAL2_RESPONSE_TIMEOUT) {
    Serial2.flush();
    this->restartMCU();
  }
}