#include "Commons/RPCManager.h"
#include "WiFi/WiFiManager.h"
#include <RPC.h>
#include "Measurement/Measurement.h"

RPCManager rpcManager;
Measurement measurement(rpcManager);
WiFiManager wifiManager(rpcManager, measurement);

bool wifiInitialized = false;

void setup() {
  Serial.begin(115200);
  RPC.begin();
  rpcManager.begin();

  measurement.setInterval(15);
  measurement.setup();

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
  measurement.loop();

  // Procesar nuevos datos del sistema desde M4
  if (rpcManager.hasNewSystemData()) {
    const SystemStatusMessage& data = rpcManager.getSystemData();
    
    // Verificar si se solicita reset de mediciones
    if (data.resetMeasurementsFlag) {
      measurement.minRead = measurement.maxRead = measurement.lastRead;
      
      // Enviar inmediatamente los valores reseteados
      rpcManager.sendMeasurementData(measurement.lastRead, measurement.minRead, measurement.maxRead);
    }

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
