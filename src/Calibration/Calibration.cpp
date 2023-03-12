#include <Arduino.h>
#include <Commons/Commons.h>
#include <Calibration/Calibration.h>

float Calibration::read() {
  if (Serial2.available()) {
    float read = Serial2.readStringUntil('\n').toFloat();

    this->lastRead = read;
    this->readValueNum++;
    this->readValueSum += this->lastRead;
  }

  if (this->lastRead < this->minRead) {
    this->minRead = this->lastRead;
  } else if (this->lastRead > this->maxRead) {
    this->maxRead = this->lastRead;
  }

  return this->lastRead;
}

void Calibration::setMinRange(float value) { this->minRange = value; }

void Calibration::setMaxRange(float value) { this->maxRange = value; }

bool Calibration::checkInRange() {
  return this->lastRead >= this->minRange && this->lastRead <= this->maxRange;
}

float Calibration::average() {
  if (this->readValueNum == 0) return 0.00f;

  return ((this->readValueSum / this->readValueNum) * 100) / 100;
}

void Calibration::reset() {
  this->minRead = this->lastRead;
  this->maxRead = this->lastRead;
  this->lastRead = 0;
  this->readValueNum = 0;
  this->readValueSum = 0;
}