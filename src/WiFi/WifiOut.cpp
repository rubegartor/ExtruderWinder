#include <Commons/Commons.h>
#include <WiFi/WifiOut.h>

/**
 * - Para enviar datos al ESP-01S se usa el puerto TX2
 * - Para recibit datos del ESP-01S se usa el puerto RX
 */

void WifiOut::put(String header, String info, String data) {
  if (Serial2.availableForWrite()) {
    Serial2.println(header + "," + info + "," + data);
  }
}

void processWifiMessages(String header, String data) {
  if (header == "IPAddress") {
    wifiOut.ipAddr = data;
  }
}

int splitString(String str, char delim, String outArr[], int maxCount) {
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
    splitString(read, ',', arr, 3);

    if (arr[0] == "WiFi") processWifiMessages(arr[1], arr[2]);
  }
}