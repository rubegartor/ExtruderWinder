#include <Arduino.h>
#include <Commons/Commons.h>
#include <Cooler/Cooler.h>

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);

void Cooler::init() {
  tempSensor.setResolution(0x1F);

  tempSensor.begin();
}

void Cooler::refresh() {
  tempSensor.setWaitForConversion(false);
  tempSensor.requestTemperatures();
  tempSensor.setWaitForConversion(true);
}

float Cooler::read() {
  return tempSensor.getTempCByIndex(0);
}

bool Cooler::isRefrigerationNeeded() {
  uint16_t tempRead = int(round(this->read()));

  if (tempRead + TEMP_THRESHOLD >= pref.getInt(TEMP_PREF, TEMP_DEFAULT_PREF)) {
    return true;
  }

  return false;
}
