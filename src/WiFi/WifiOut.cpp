#include <Commons/Commons.h>
#include <WiFi/WifiOut.h>

/**
 * - Para enviar datos al ESP-01S se usa el puerto TX2
 * - Para recibit datos del ESP-01S se usa el puerto RX
 */

bool WifiOut::isConnected() { return this->connected; }

void WifiOut::put(String header, String info, String data) {
  if (Serial2.availableForWrite()) {
    Serial2.println(header + "," + info + "," + data);
  }
}

void WifiOut::processWifiMessages(String header, String data) {
  if (header == "IPAddress") {
    this->ipAddr = data;
    this->connected = true;
  }

  if (header == "requestMeasuring") {
    this->put("Extruder", "Measuring",
              (String)measuring.minRead + "," + (String)measuring.maxRead);
  }
}

int _splitString(String str, char delim, String outArr[], int maxCount) {
  int count = 0;
  int pos = 0;
  int lastPos = 0;

  while (pos != -1 && count < maxCount) {
    pos = str.indexOf(delim, lastPos);
    if (pos == -1) {
      outArr[count++] = str.substring(lastPos);
    } else {
      outArr[count++] = str.substring(lastPos, pos);
      lastPos = pos + 1;
    }
  }

  return count;
}

void WifiOut::receive() {
  if (Serial.available() > 0) {
    String read = Serial.readStringUntil('\n');
    read.trim();

    String arr[3];
    _splitString(read, ',', arr, 3);

    if (arr[0] == "WiFi") this->processWifiMessages(arr[1], arr[2]);
  }
}