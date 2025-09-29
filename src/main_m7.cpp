#include "Commons/RPCManager.h"
#include "WiFi/WiFiManager.h"
#include <RPC.h>

RPCManager rpcManager;
WiFiManager wifiManager(rpcManager);

bool wifiInitialized = false;

void setup() {
  Serial.begin(115200);
  RPC.begin();
  rpcManager.begin();

  while (!rpcManager.hasNewSystemData()) {
    rpcManager.loop();
    delay(10);
  }

  const SystemStatusMessage& data = rpcManager.getSystemData();
  
  if (data.wifiEnabled) {
    wifiManager.begin();
    wifiInitialized = true;
  }
  
  rpcManager.markSystemDataProcessed();
}

void loop() {
  rpcManager.loop();

  // Procesar nuevos datos del sistema desde M4
  if (rpcManager.hasNewSystemData()) {
    const SystemStatusMessage& data = rpcManager.getSystemData();
    
    // Gestionar el estado del WiFi según la configuración del M4
    if (data.wifiEnabled && !wifiInitialized) {
      wifiManager.begin();
      wifiInitialized = true;
    } else if (!data.wifiEnabled && wifiInitialized) {
      wifiManager.stop();
      wifiInitialized = false;
    }
    
    rpcManager.markSystemDataProcessed();
  }

  // Ejecutar loop del WiFi Manager si está habilitado
  if (wifiInitialized) {
    wifiManager.loop();
  }
}
