#include <Commons/Commons.h>
#include <Communication/Communication.h>
#include <Wire.h>

typedef struct struct_message {
    String key;
    String data;
} struct_message;

struct_message sendingData;
struct_message incomingData;

void _sendEvent(String eventName, String eventData) {
  String toSend = "\0";

  sendingData.key = eventName;
  sendingData.data = eventData;

  toSend = eventName + ";" + eventData;

  Wire.beginTransmission(0x60);
  Wire.write(toSend.c_str());
  Wire.endTransmission();
}

void Communication::sendEvent(String eventName, String eventData) {
  _sendEvent(eventName, eventData);
}

void Communication::sendTaskedEvents() {
  this->sendEvent("pSpeed", String(puller.speed));
  this->sendEvent("eWeight", String(getExtrudedWeight()));
  this->sendEvent("eTime", getTime(millis() - millisOffset));
  this->sendEvent("temp", String(cooler.read()));
}

void Communication::sendFastTaskedEvents() {
  if (aligner.isHomed()) {
    this->sendEvent("position", String(aligner.getCurrentPosition()).c_str());
  }
}

void convertToStruct(const String &input, char delimiter) {
  int delimiterPos = input.indexOf(delimiter);

  incomingData.key = input.substring(0, delimiterPos);
  incomingData.data = input.substring(delimiterPos + 1);
}

void Communication::requestData() {
  uint8_t bytesReceived = Wire.requestFrom(SLAVE_ADDRESS, 32);

  String read = "";

  while (Wire.available()) {
    for (uint8_t i = 0; i < bytesReceived; i++) {
      read += (char)Wire.read();
    }
  }

  read.trim();

  convertToStruct(read, ';');

  if (incomingData.key == "resetMeasure") {
    puller.totalRevs = 0;
    spooler.totalRevs = 0;
    aligner.lastTotalRevs = 0;
    millisOffset = millis();
    measuring.reset();
  }

  if (incomingData.key == "moveLft") {
    long steps = (STEPS_PER_CM * (filamentDiameter / 10)) * 3;
    if (incomingData.data != "") {
      steps = (long)(STEPS_PER_CM * (filamentDiameter / 10));
    }

    aligner.moveTo(-steps);
  }

  if (incomingData.key == "moveRgt") {
    long steps = (STEPS_PER_CM * (filamentDiameter / 10)) * 3;
    if (incomingData.data != "") {
      steps = (long)(STEPS_PER_CM * (filamentDiameter / 10));
    }

    aligner.moveTo(steps);
  }

  if (incomingData.key == "startAligner") {
    aligner.startAlignerPosition();
  }

  if (incomingData.key == "setAligner") {
    aligner.setAlignerPosition();
  }

  if (incomingData.key == "reqMinRead") {
    _sendEvent("minRead", (String)measuring.minRead);
  }

  if (incomingData.key == "reqMaxRead") {
    _sendEvent("maxRead", (String)measuring.maxRead);
  }

  if (incomingData.key == "reqMinPSpeed") {
    _sendEvent("minPSpeed", (String)pidPuller.getMinPullerSpeed());
  }

  if (incomingData.key == "reqMaxPSpeed") {
    _sendEvent("maxPSpeed", (String)pidPuller.getMaxPullerSpeed());
  }

  if (incomingData.key == "reqDiameter") {
    _sendEvent("diameter", (String)pref.getFloat(FILAMENT_DIAMETER_MODE_PREF, DEFAULT_FILAMENT_DIAMETER));
  }

  if (incomingData.key == "reqAutostop") {
    _sendEvent("autostop", (String)pref.getFloat(AUTOSTOP_THRESHOLD_PREF, AUTOSTOP_THRESHOLD_DEFAULT));
  }

  if (incomingData.key == "reqMaxPosition") {
    int16_t maxPosition = ALIGNER_MAX_DISTANCE;

    if (aligner.isPositioned()) {
      maxPosition = aligner.endPos;
    }

    _sendEvent("maxPosition", (String)maxPosition);
    _sendEvent("position", (String)aligner.getCurrentPosition());
  }

  if (incomingData.key == "reqMinPosition") {
    int16_t minPosition = ALIGNER_START_POSITION;

    if (aligner.isPositioned()) {
      minPosition = aligner.startPos;
    }

    _sendEvent("minPosition", (String)minPosition);
  }

  if (incomingData.key == "reqPolymer") {
    uint8_t polymerIndex = 0;
    Polymer actualPolymer = stringToPolymer(
        pref.getString(SELECTED_POLYMER_PREF, polymers[0].name));

    for (uint8_t i = 0; i < POLYMER_NUMBER; i++) {
      if (actualPolymer.name == polymers[i].name) {
        polymerIndex = i;
        break;
      }
    }

    _sendEvent("polymer", (String)polymerIndex);
  }

  if (incomingData.key == "setMinSpeed") {
    pidPuller.updateMinPullerSpeed(incomingData.data.toInt());
  }

  if (incomingData.key == "setMaxSpeed") {
    pidPuller.updateMaxPullerSpeed(incomingData.data.toInt());
  }

  if (incomingData.key == "setPolymer") {
    pref.putString(SELECTED_POLYMER_PREF, incomingData.data);
  }

  if (incomingData.key == "setDiameter") {
    filamentDiameter = incomingData.data.toFloat() / 100;
    pref.putFloat(FILAMENT_DIAMETER_MODE_PREF, filamentDiameter);
    pidPuller.updateSetPoint(filamentDiameter);
  }

  if (incomingData.key == "setAutostop") {
    pref.putFloat(AUTOSTOP_THRESHOLD_PREF, incomingData.data.toFloat() / 100);
  }

  if (incomingData.key == "setTemp") {
    pref.putInt(TEMP_PREF, incomingData.data.toInt());
  }

  if (incomingData.key == "homeAligner") {
    aligner.resetHome();
    _sendEvent("maxPosition", (String)ALIGNER_MAX_DISTANCE);
    _sendEvent("minPosition", (String)0);
  }

  if (incomingData.key == "addRPos") {
    aligner.endPos += (long)(STEPS_PER_CM * (filamentDiameter / 10));
    _sendEvent("maxPosition", (String)aligner.endPos);
  }

  if (incomingData.key == "rmRPos") {
    aligner.endPos -= (long)(STEPS_PER_CM * (filamentDiameter / 10));
    _sendEvent("maxPosition", (String)aligner.endPos);
  }

  if (incomingData.key == "addLPos") {
    aligner.startPos += (long)(STEPS_PER_CM * (filamentDiameter / 10));
    _sendEvent("minPosition", (String)aligner.startPos);
  }

  if (incomingData.key == "rmLPos") {
    aligner.startPos -= (long)(STEPS_PER_CM * (filamentDiameter / 10));
    _sendEvent("minPosition", (String)aligner.startPos);
  }

  if (incomingData.key == "reqPositioned") {
    _sendEvent("positioned", aligner.isPositioned() ? "1": "0");
  }

  if (incomingData.key == "reqStAutoStop") {
    _sendEvent("stAutoStop", (String)measuring.autoStopStatus);
  }

  if (incomingData.key == "reqTemp") {
    _sendEvent("stTemp", (String)pref.getInt(TEMP_PREF, TEMP_DEFAULT_PREF));
  }

  if (incomingData.key == "reboot") {
    digitalWrite(BUZZER_PIN, LOW);
    ESP.restart();
  }
}

void Communication::init() {
  Wire.begin();

  this->sendEvent("resetScr", "");
}