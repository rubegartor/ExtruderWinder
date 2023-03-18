#include <Arduino.h>
#include <Commons/Commons.h>
#include <Measuring/Measuring.h>

float Measuring::read() {
  if (Serial2.available()) {
    this->lastRead = Serial2.readStringUntil('\n').toFloat();
    this->readValueNum++;
    this->readValueSum += this->lastRead;
  }

  if (this->lastRead < this->minRead) {
    this->minRead = this->lastRead;
  } else if (this->lastRead > this->maxRead) {
    this->maxRead = this->lastRead;
  }

  Serial2.println(this->lastRead);

  return this->lastRead;
}

void Measuring::setMinRange(float value) { this->minRange = value; }

void Measuring::setMaxRange(float value) { this->maxRange = value; }

bool Measuring::checkInRange() {
  return this->lastRead >= this->minRange && this->lastRead <= this->maxRange;
}

float Measuring::average() {
  if (this->readValueNum == 0) return 0.00f;

  return ((this->readValueSum / this->readValueNum) * 100) / 100;
}

void Measuring::reset() {
  this->minRead = this->lastRead;
  this->maxRead = this->lastRead;
  this->lastRead = 0;
  this->readValueNum = 0;
  this->readValueSum = 0;

  Serial2.println("reset");
}