#include "EventLog.h"
#include <mbed.h>
#include <rtos.h>
#include <stddef.h>
#include "kvstore_global_api.h"

#define LOG_KEY        "evlog"
#define LOG_MAGIC      0x45564C31UL  // "EVL1": cambiarlo si cambia LogEntry
// Desgaste de la flash: un evento que se repite se agrupa en la misma entrada durante
// LOG_COALESCE_MS, y se escribe como mucho cada LOG_FLUSH_MS (LOG_URGENT_MS si es un fallo grave).
#define LOG_COALESCE_MS 60000
#define LOG_FLUSH_MS    60000
#define LOG_URGENT_MS   5000
#define HANG_MS         5000

volatile uint32_t g_beatUi = 0, g_beatMotor = 0;

struct Blob {
  uint32_t magic;
  uint16_t boot;
  uint8_t  n;
  uint8_t  pad;
  LogEntry e[LOG_MAX];  // orden cronológico
};

static rtos::Mutex mtx;
static Blob     ram;               // registro vivo
static Blob     out;               // copia para escribir en flash sin tener el mutex
static uint16_t counts[SRC_SYS + 1][EV_COUNT];
static uint32_t version = 0;
static bool     dirty = false, urgent = false;

static bool isUrgent(uint8_t kind) {
  return kind == EV_BOOT || kind == EV_TMC_LOST || kind == EV_TMC_DRVERR || kind == EV_TMC_UVCP ||
         kind == EV_TENSIONER_LOST || kind == EV_UI_HANG || kind == EV_MOTOR_HANG;
}

void logEvent(uint8_t src, uint8_t kind, uint32_t a, uint32_t b, uint32_t c) {
  if (src > SRC_SYS || kind >= EV_COUNT) return;
  uint32_t now = millis();
  mtx.lock();
  if (counts[src][kind] < 0xFFFF) counts[src][kind]++;

  // Agrupar con la última entrada del mismo tipo si es reciente: así una ráfaga de
  // ruido no expulsa del registro lo que pasó antes del reinicio.
  LogEntry* e = nullptr;
  for (int i = ram.n - 1; i >= 0; i--) {
    LogEntry& x = ram.e[i];
    if (x.boot == ram.boot && x.src == src && x.kind == kind) {
      if (now - x.ms < LOG_COALESCE_MS) e = &x;
      break;
    }
  }
  if (!e) {
    if (ram.n == LOG_MAX) memmove(&ram.e[0], &ram.e[1], sizeof(LogEntry) * (LOG_MAX - 1));
    else ram.n++;
    e = &ram.e[ram.n - 1];
    e->boot = ram.boot;
    e->src = src;
    e->kind = kind;
    if (isUrgent(kind)) urgent = true;
  }
  e->ms = now;
  e->a = a; e->b = b; e->c = c;
  e->count = counts[src][kind];
  dirty = true;
  version++;
  mtx.unlock();
}

uint8_t eventLogSnapshot(LogEntry* dst) {
  mtx.lock();
  uint8_t n = ram.n;
  for (uint8_t i = 0; i < n; i++) dst[i] = ram.e[n - 1 - i];
  mtx.unlock();
  return n;
}

uint16_t eventLogBoot() { return ram.boot; }
uint32_t eventLogVersion() { return version; }

void eventLogClear() {
  mtx.lock();
  ram.n = 0;
  memset(counts, 0, sizeof(counts));
  dirty = urgent = true;
  version++;
  mtx.unlock();
}

// Hilo propio: la escritura en flash puede tardar (borrado de sector) y no debe frenar
// ni los motores ni la UI. Prioridad Normal como los demás: loop() nunca cede la CPU,
// así que un hilo de menor prioridad no llegaría a ejecutarse.
static void logTask() {
  uint32_t lastFlush = 0;
  bool hung[2] = { false, false };
  while (true) {
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(500));
    uint32_t now = millis();

    const uint32_t beats[2] = { g_beatUi, g_beatMotor };
    for (int i = 0; i < 2; i++) {
      uint32_t age = now - beats[i];
      if (!hung[i] && age > HANG_MS) {
        hung[i] = true;
        logEvent(SRC_SYS, i ? EV_MOTOR_HANG : EV_UI_HANG, age);
      } else if (age < 1000) {
        hung[i] = false;
      }
    }

    mtx.lock();
    bool due = dirty && (now - lastFlush >= LOG_FLUSH_MS || (urgent && now - lastFlush >= LOG_URGENT_MS));
    if (due) {
      out = ram;
      dirty = urgent = false;
    }
    mtx.unlock();
    if (!due) continue;

    lastFlush = now;
    size_t size = offsetof(Blob, e) + out.n * sizeof(LogEntry);
    if (kv_set(LOG_KEY, &out, size, 0) != MBED_SUCCESS) {
      mtx.lock();
      dirty = true;  // reintentar en la próxima ventana
      mtx.unlock();
    }
  }
}

void eventLogBegin() {
  // Lo leído de flash puede estar corrupto: validar tamaño, cabecera y cada entrada.
  size_t got = 0;
  mtx.lock();
  ram.magic = LOG_MAGIC;
  if (kv_get(LOG_KEY, &out, sizeof(out), &got) == MBED_SUCCESS && got >= offsetof(Blob, e) &&
      out.magic == LOG_MAGIC && out.n <= LOG_MAX && got == offsetof(Blob, e) + out.n * sizeof(LogEntry)) {
    ram.boot = out.boot;
    for (uint8_t i = 0; i < out.n; i++) {
      if (out.e[i].src <= SRC_SYS && out.e[i].kind < EV_COUNT) ram.e[ram.n++] = out.e[i];
    }
  }
  ram.boot++;
  mtx.unlock();

  logEvent(SRC_SYS, EV_BOOT, mbed::ResetReason::get(), mbed::ResetReason::get_raw());

  g_beatUi = g_beatMotor = millis();
  rtos::Thread* t = new rtos::Thread(osPriorityNormal, 8192);
  t->start(logTask);
}
