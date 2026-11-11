#pragma once

#include <Arduino.h>
#include <TMC51X0.hpp>
#include "Commons/EventLog.h"

// 2 MHz: el datasheet (4.3) limita SCK a 4 MHz con el reloj interno. Margen x2 frente a
// ruido/reflexiones en un bus compartido por 3 drivers. El tráfico por ciclo es mínimo.
#define TMC5160_SPI_FREQ 2000000

// Tramas sanas seguidas con el flag de reset antes de darlo por real sin ver TOFF = 0.
#define TMC_FAIL_THRESHOLD 3

enum class TmcHealth {
  Ok,      // Driver sano: se puede operar este ciclo.
  Glitch,  // Trama SPI corrupta o bus caído: saltar este ciclo sin tocar nada.
  Lost     // El chip se reinició (config y XACTUAL/XTARGET a 0): quien llama debe reescribir sus parámetros.
};

// Origen de los eventos en el registro (Commons/EventLog.h).
enum TmcId : uint8_t { TMC_PULLER = 0, TMC_ALIGNER = 1, TMC_SPOOLER = 2 };

struct TmcGuard {
  uint32_t cfg[4] = {};   // última config buena de GCONF, CHOPCONF, RAMPMODE, SW_MODE (lo que escribimos)
  bool     valid = false;
  uint8_t  resetSeen = 0;
};

// El SPI del TMC5160 no lleva CRC ni paridad (datasheet 4.1): un bit corrupto es un valor
// válido más, y un pulso de ruido en SCK desplaza la trama y escribe en otro registro.
// La librería, además, vuelve a escribir en el chip lo que lee (su espejo de registros se
// actualiza con cada lectura y sus funciones hacen leer-modificar-escribir sobre él), así
// que una lectura corrupta de CHOPCONF/GCONF acababa grabada como configuración real
// (micropasos, sentido, modo de rampa...) hasta apagar la máquina.
//
// Reglas de esta función, que sustituye a la recuperación de la librería:
//  - Ninguna lectura se da por buena para escribirla de vuelta. La referencia es siempre lo
//    último que escribimos nosotros (g.cfg).
//  - Cada ciclo se comparan los 4 registros de config legibles con esa referencia y se
//    reescribe el que difiera: repara tanto una lectura corrupta como una escritura corrupta.
//  - Sin comunicación no se escribe nada (antes se reinicializaba a ciegas y se ponía
//    XACTUAL a 0: el Aligner perdía la referencia por una ráfaga de ruido).
//  - Reset real = flag de reset (GSTAT, se borra escribiendo 1) + CHOPCONF.TOFF = 0, que es
//    su valor tras reset. Solo entonces se pierde la posición.
inline TmcHealth checkTmcHealth(TMC51X0 &motor, TmcGuard &g, TmcId id) {
  using tmc51x0::Registers;
  static const Registers::RegisterAddress CFG[4] = {
    Registers::GconfAddress, Registers::ChopconfAddress, Registers::RampmodeAddress, Registers::SwModeAddress
  };
  Registers &reg = motor.registers;

  // El espejo de la librería es fiable (= lo último escrito) salvo que lo haya borrado:
  // lo hace por su cuenta al ver el bit de reset en el byte de estado de cualquier trama.
  if (!reg.resyncRequired()) {
    for (uint8_t i = 0; i < 4; i++) g.cfg[i] = reg.getStored(CFG[i]);
    g.valid = true;
  }

  Registers::Gstat gstat;
  gstat.raw = reg.read(Registers::GstatAddress);
  Registers::Ioin ioin;
  ioin.raw = reg.read(Registers::IoinAddress);
  // GSTAT solo tiene 3 bits e IOIN.VERSION es fijo (0x30): cualquier otra cosa es una trama corrupta.
  if ((gstat.raw & ~0x7UL) || ioin.version() != 0x30) {
    logEvent(id, EV_TMC_GLITCH, gstat.raw, ioin.raw);
    return TmcHealth::Glitch;
  }

  Registers::Chopconf chop;
  chop.raw = reg.read(Registers::ChopconfAddress);

  if (!gstat.reset()) {
    g.resetSeen = 0;
  } else if (chop.toff() == 0 || ++g.resetSeen >= TMC_FAIL_THRESHOLD) {
    // El flag solo se borra aquí, así que verlo en varias tramas sanas también es un reset real.
    g.resetSeen = 0;
    g.valid = false;
    logEvent(id, EV_TMC_LOST, gstat.raw, chop.raw);
    reg.assumeDeviceReset();                       // espejo = valores de reset, como el chip
    reg.write(Registers::GstatAddress, 0x7);
    reg.read(Registers::GstatAddress);             // el byte de estado refleja el borrado en la trama siguiente
    reg.clearResyncRequired();
    return TmcHealth::Lost;
  }

  if (reg.resyncRequired()) {
    // Espejo borrado por un bit de reset espurio: reconstruirlo sin leer nada del chip.
    // Primero los 4 registros legibles desde la copia buena (así TOFF nunca pasa por 0) y
    // después setup(), que reescribe el resto desde los parámetros que guarda la librería.
    reg.clearResyncRequired();
    logEvent(id, EV_TMC_STATUS);
    if (g.valid) {
      for (uint8_t i = 0; i < 4; i++) reg.write(CFG[i], g.cfg[i]);
    }
    motor.driver.setup();
    motor.controller.setup();
  } else {
    for (uint8_t i = 0; i < 4; i++) {
      uint32_t actual = (CFG[i] == Registers::ChopconfAddress) ? chop.raw : reg.read(CFG[i]);
      if (actual != g.cfg[i]) {
        logEvent(id, EV_TMC_REPAIR, CFG[i], g.cfg[i], actual);
        reg.write(CFG[i], g.cfg[i]);
      }
    }
  }

  if (gstat.uv_cp()) logEvent(id, EV_TMC_UVCP, gstat.raw);
  if (gstat.drv_err()) {
    // DRV_STATUS dice cuál: sobretemperatura (ot) o corto a masa / a VS en una fase.
    logEvent(id, EV_TMC_DRVERR, gstat.raw, reg.read(Registers::DrvStatusAddress));
    // Apagado por sobretemperatura/cortocircuito: el puente solo se rearma con TOFF=0 -> TOFF.
    motor.driver.disable();
    motor.driver.enable();
  }
  if (gstat.raw & 0x6) reg.write(Registers::GstatAddress, gstat.raw & 0x6);

  return TmcHealth::Ok;
}
