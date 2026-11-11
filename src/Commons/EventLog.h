#pragma once

#include <Arduino.h>

// Registro de eventos persistente en flash (KVStore). Sobrevive a reinicios para poder
// revisar qué pasó antes de un cuelgue. No depende de la UI: los eventos se registran
// donde ocurren (motorTask, UI) y un hilo propio los guarda y vigila que la UI y el
// motorTask sigan vivos.

#define LOG_MAX 32
#define SRC_SYS 3   // origen de los eventos que no son de un driver (0..2 = TmcId)

enum LogKind : uint8_t {
  EV_BOOT,            // a = ResetReason, b = registro de causa de reset en bruto
  EV_TMC_GLITCH,      // trama corrupta descartada. a = GSTAT, b = IOIN leídos
  EV_TMC_STATUS,      // bit de reset espurio en el byte de estado SPI
  EV_TMC_REPAIR,      // registro de config distinto de lo escrito. a = dirección, b = escrito, c = leído
  EV_TMC_LOST,        // reset real del chip. a = GSTAT, b = CHOPCONF
  EV_TMC_DRVERR,      // apagado por temperatura o corto. a = GSTAT, b = DRV_STATUS
  EV_TMC_UVCP,        // subtensión en la bomba de carga. a = GSTAT
  EV_TENSIONER_LOST,  // el ultrasonidos dejó de devolver eco
  EV_SPOOL_FULL,      // a = umbral en gramos
  EV_UI_HANG,         // loop() (UI + calibre) sin latido. a = ms
  EV_MOTOR_HANG,      // motorTask sin latido. a = ms
  EV_COUNT
};

struct LogEntry {
  uint32_t ms;        // millis() de la última aparición dentro de ese arranque
  uint32_t a, b, c;   // detalle, según el tipo
  uint16_t boot;      // número de arranque
  uint16_t count;     // apariciones de ese tipo en ese arranque
  uint8_t  src;
  uint8_t  kind;
};

// Latidos: los escriben loop() y motorTask; el hilo del registro los vigila.
extern volatile uint32_t g_beatUi, g_beatMotor;

void     eventLogBegin();   // al final de setup(): carga de flash, registra el arranque y lanza el hilo
void     logEvent(uint8_t src, uint8_t kind, uint32_t a = 0, uint32_t b = 0, uint32_t c = 0);
uint8_t  eventLogSnapshot(LogEntry* out);  // copia, más reciente primero; devuelve cuántas
uint16_t eventLogBoot();
uint32_t eventLogVersion();                // cambia con cada evento: para saber cuándo repintar
void     eventLogClear();
