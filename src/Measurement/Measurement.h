#pragma once

#include <Arduino.h>
#include "Commons/TimedComponent.h"
#include "Commons/RPCManager.h"

#define MITUTOYO_CLK_DEBOUNCE 280

#define MEASUREMENT_LIMIT 10.60f
#define FIRST_DIGIT_ERROR_THRESHOLD 0.95f  // Umbral para detectar errores del primer dígito
#define MAX_REJECTED_READS 3                // Máximo número de lecturas rechazadas antes de aceptar

class Measurement : public TimedComponent {    
  public:
    Measurement(RPCManager& rpcMgr) : TimedComponent(), rpcManager(rpcMgr) {}

    float lastRead = 0.00f;
    float minRead = 0.00f;
    float maxRead = 0.00f;
    unsigned long lastUpdateTime = 0;
    float previousValidRead = 0.00f;  // Almacena la última lectura válida para comparación
    uint8_t rejectedReadCount = 0;    // Contador de lecturas rechazadas consecutivas

    void setup();
    void execute() override;

  private:
    RPCManager& rpcManager;
    bool isFirstDigitError(float newValue, float previousValue);
};