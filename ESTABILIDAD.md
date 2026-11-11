# Estabilidad: revisión de fallos aleatorios y panics

Revisión del 2026-10-18 (rama `spi-race`). Objetivo: que el sistema funcione de forma continua sin reinicios ni comportamientos erráticos de los TMC5160.

## Revisión del 2026-11-11, contra el datasheet TMC5160A rev. 1.18

El datasheet está en `.pio/libdeps/giga_r1_m7/TMC51X0/datasheet/`. Las secciones citadas son de esa revisión.

### Qué pasa

El uso del SPI es correcto según el datasheet (modo 3, 2 MHz frente a un máximo de 4 MHz con reloj interno, tramas de 40 bits). El problema es qué hace el firmware cuando una trama llega mal:

1. **El SPI del TMC5160 no tiene CRC ni paridad** (4.1). Un bit cambiado es un valor válido más. Al subir CSN el chip ejecuta los últimos 40 bits recibidos (4.2), así que un pulso de ruido en SCK desplaza la trama y puede escribir en otro registro. Las entradas solo filtran pulsos de menos de 20 ns (4.3).
2. **La librería reescribe en el chip lo que lee.** Guarda una copia de cada registro que se actualiza con cada lectura, y sus funciones (`enable`, `enableStallStop`, `setup`...) modifican esa copia y la escriben. `checkTmcHealth()` leía CHOPCONF en cada ciclo (unas 35 lecturas por segundo entre los tres drivers). Una lectura corrupta que coincidiera con una reescritura quedaba grabada como configuración real: micropasos (MRES), sentido de giro, modo de rampa. A partir de ahí las posiciones y velocidades dejan de tener sentido hasta apagar la máquina.
3. **Tres ciclos sin comunicación se trataban como "driver perdido"**: se reescribía todo a ciegas sobre un bus que no funcionaba y se ponía XACTUAL a 0. Una ráfaga de ruido de 240 ms mandaba el Aligner a hacer homing en mitad de la bobina.
4. **El flag de reset se borraba aunque la lectura fuera basura**, con lo que un reset real podía pasar desapercibido.

**Origen probable de los resets reales:** el chip vuelve a sus valores de fábrica si VCC_IO cae por debajo del umbral, que puede llegar a **3,0 V** (mín. 2,0 / típ. 2,5 / máx. 3,0 V, con solo 10 µs de retardo; capítulo 25 y tabla 28.2). Con VCC_IO a 3,3 V desde el Giga el margen es de 0,3 V en el peor caso: un rebote de masa por la corriente de los motores basta.

### Qué se ha cambiado

Compila; **pendiente de probar en la máquina.**

| Cambio | Archivo |
|---|---|
| `checkTmcHealth()` reescrito. La referencia es siempre lo último que escribió el firmware, nunca una lectura. Cada ciclo compara GCONF, CHOPCONF, RAMPMODE y SW_MODE con esa referencia y reescribe el que difiera. Sin comunicación no escribe nada ni toca la posición. Reset real = flag de reset + `CHOPCONF.TOFF = 0`; el flag solo se borra en una trama sana. | `src/Motors/Stepper.h` |
| Tras un reset real, cada motor reescribe sus propios parámetros (`applyParameters()`), en vez de los que la librería recordaba. También lo hace al final del arranque, porque `endRampToZeroVelocity()` restaura el modo de rampa que leyó del chip. | Puller / Spooler / Aligner |
| Aligner: el stall y el "posición alcanzada" que fijan el origen y los límites exigen dos lecturas seguidas iguales. | `src/Motors/Aligner.cpp` |
| Puller: el filtro de cordura de XACTUAL pasa de 21 vueltas a 2 (el comentario calculaba mal la velocidad máxima). | `src/Motors/Puller.cpp` |
| Alertas nuevas por driver, con el total acumulado: lectura SPI corrupta, config corregida, baja tensión en motores (`uv_cp`). "Driver reiniciado" ahora significa reset real del chip. | `src/UI/ui.cpp`, `Stepper.h` |

### Cómo leer las alertas

Se guardan en flash (`src/Commons/EventLog.cpp`, clave KVStore `evlog`, 32 entradas) y sobreviven a un reinicio. Cada una lleva el número de arranque, el tiempo desde ese arranque y los registros leídos del driver (GSTAT, IOIN, CHOPCONF, DRV_STATUS, o el valor escrito y el leído en una corrección). Cada arranque deja una entrada con su causa (encendido, botón de reset, caída de tensión, watchdog...). Un hilo propio vigila que `loop()` y el `motorTask` sigan vivos y registra "Pantalla sin responder" / "Control de motores sin responder" si dejan de latir 5 s.

Para no gastar la flash, un evento que se repite se agrupa en la misma entrada durante 60 s, y se escribe como mucho cada 60 s (5 s si es un fallo grave). Tras un cuelgue se pueden perder los últimos segundos.

Sirven para localizar el fallo de hardware, que el firmware solo puede tolerar:

- **"lectura SPI corrupta"** en un solo driver: su cable MISO/CS o su masa. En los tres a la vez: SCK/MOSI o la masa común.
- **"config corregida"**: una escritura llegó mal al chip (o una lectura pasó los filtros). Antes esto era el fallo permanente.
- **"reinicio por baja tensión"**: el chip se reseteó de verdad. Mirar VCC_IO y la masa de ese driver.
- **"baja tensión en motores"**: la fuente o el condensador de VM de ese driver se quedan cortos.
- Si "config corregida" sube sin parar desde el arranque en un driver, avisar: sería un registro que no se lee igual que se escribe.

### Límites

- Los registros de solo escritura (corrientes, AMAX, umbrales, PWMCONF) no se pueden verificar. Una escritura corrupta ahí sigue sin detectarse. VMAX y XTARGET se reescriben continuamente, así que se corrigen solos.
- Si el chip se resetea, la posición se pierde y el Aligner hace homing. Eso solo lo evita el hardware.

### Valores fuera de rango (no cambiados: afectan al movimiento y al homing)

Con el reloj de 12 MHz y las unidades configuradas, estos valores no caben en sus registros (6.3.1). El datasheet no dice qué hace el chip con los bits que sobran; lo previsible es que los ignore, y entonces el motor no hace lo que dice el código:

| Motor | Parámetro | Valor pedido | Valor en chip | Máximo del registro |
|---|---|---|---|---|
| Aligner | VSTART y VSTOP = 180 | 1 610 612 | 37 748 (≈ 4,2 unidades) | 262 143 (18 bits, ≈ 29 unidades) |
| Aligner | V1 = 180 | 1 610 612 | 562 036, por encima de VMAX | 1 048 575 (20 bits) |
| Puller | AMAX = 1 000 000 | 203 611 | 7 003 (≈ 34 000 unidades) | 65 535 (16 bits) |

Funciona así desde siempre, por eso no se toca sin probar. Lo razonable es escribir el valor efectivo (Aligner: VSTART = VSTOP = 4, V1 = 0; Puller: AMAX = 34 000) y comprobar el homing.

Otros puntos del datasheet:

- El homing consulta `status_sg` cada 80 ms. El datasheet avisa de que consultando ese flag se pueden perder eventos (6.3.2.2). El flag pensado para esto es `event_stop_sg`.
- `sg_stop` se activa con el motor parado, y el datasheet pide no activarlo durante el arranque (6.3.2.1). Lo cubre TCOOLTHRS, pero es el primer sitio donde mirar si el homing da falsos stalls.
- El Puller tiene desactivada la protección de cortocircuito a masa (`withShortToGroundProtectionEnabled(false)`).

## Revisión del 2026-10-18

**Causa principal de que la máquina "se vuelva loca":** la librería TMC51X0 (v4.0.3) llama a `recoverFromDeviceReset()` ante **una sola** lectura SPI corrupta (IOIN o GSTAT con ruido) desde `recoverIfUnhealthy()` y `readHealthStatus()` + `recoverIfNeeded()`. Esa función:

- deshabilita y rehabilita el puente (TOFF=0 → TOFF), lo que provoca un tirón en el motor;
- pone **XACTUAL y XTARGET a 0**. En el Aligner eso significa perder la referencia en mitad de la bobina (sin re-homing → el carro acaba contra el final de carrera) o hacer un re-homing inesperado.

**Otras lecturas SPI sueltas usadas para decidir movimientos:**

- El Aligner releía XTARGET por SPI en `onSpoolerRevolution()` y `moveTo()`. Una lectura corrupta mandaba el carro a un extremo.
- El Spooler aceptaba saltos de hasta 5 vueltas: un bit erróneo movía el Aligner 5 pasos.

**Tensor:** NewPing devuelve `0` (NO_ECHO) cuando pierde el eco. Se trataba como distancia 0 (≤ `TENSIONER_MIN_HARD_LIMIT`), así que cada eco perdido frenaba el Spooler en seco.

**Cuelgues y panics:**

- Parche de `Arduino_H7_Video`: si `realloc` fallaba, el flush retornaba sin `lv_display_flush_ready()`. LVGL esperaba en bucle infinito y se congelaban la UI y la medición del calibre (mismo hilo).
- Valores corruptos en flash: un índice de plástico fuera de rango leía basura (posible HardFault al construir la UI → bucle de arranque). Un diámetro 0 (permitido por el spinbox) dejaba `aligner_to_move = 0` (divisiones por cero).
- El core mbed del Giga tiene `MBED_CONF_PLATFORM_FATAL_ERROR_AUTO_REBOOT_ENABLED = 0`: ante un panic la placa se queda parada (LED rojo) y **los TMC5160 siguen girando a la última VMAX**, porque tienen su propio generador de rampas. **Sin resolver**, ver pendientes.

## Hecho

Compila sin errores: RAM 24,1 %, Flash 36,7 %. **Pendiente de probar en la máquina.**

| # | Cambio | Archivo |
|---|---|---|
| 1 | Nuevo `checkTmcHealth()`, usado por los 3 motores en lugar de la recuperación de la librería. Reescrito el 2026-11-11 (ver arriba). | `src/Motors/Stepper.h` |
| 2 | SPI de 4 MHz a 2 MHz (margen frente al límite del TMC5160 con reloj interno y frente al ruido) | `src/Motors/Stepper.h` |
| 3 | Aligner: XTARGET guardado en RAM (`targetPosition` / `writeTarget()`), ya no se relee por SPI | `src/Motors/Aligner.cpp/.h` |
| 4 | Aligner: los límites de calibración se leen hasta que dos lecturas coinciden (`readStoppedPosition()`) | `src/Motors/Aligner.cpp` |
| 5 | Aligner y Spooler: eliminados `enabled()` y la lógica duplicada de `reinit()`; lo cubre el chequeo común | Aligner / Spooler |
| 6 | Spooler: el filtro de cordura pasa de 5 vueltas a 1,5. La posición se cuenta en micropasos con aritmética modular, así que el desbordamiento de XACTUAL (~2 h a velocidad máxima) ya no pierde una vuelta. | `src/Motors/Spooler.cpp/.h` |
| 7 | Puller: delta de posición con aritmética modular (sin comportamiento indefinido al desbordar) | `src/Motors/Puller.cpp/.h` |
| 8 | Tensor: un eco perdido mantiene la última lectura. Tras `TENSIONER_MAX_MISSED_ECHOES` (20 × 100 ms) seguidos se fuerza 0 y se para el Spooler (sensor caído) | `src/Tensioner/Tensioner.cpp/.h` |
| 9 | Validación de valores cargados de flash: tipo de plástico y diámetro (`DIAMETER_MIN` / `DIAMETER_MAX`) | `src/Commons/globals.cpp/.h` |
| 10 | Spinbox del diámetro limitado a 0,50–3,00 mm (antes permitía 0) | `src/UI/components/homeTab.cpp` |
| 11 | `motorTask` cede la CPU 1 ms por vuelta (`ThisThread::sleep_for`) | `src/main_m7.cpp` |
| 12 | Parche de `Arduino_H7_Video`: llama a `lv_display_flush_ready()` también cuando falla `realloc` | `scripts/apply_h7_video_fix.py` |

## Pendiente

### Verificación

- [ ] Probar en la máquina: homing, calibración del carrete, movimiento manual y un bobinado largo de varias horas.
- [ ] Hacer commit de los cambios.

### Robustez (requiere decisión)

- [ ] **Watchdog** (`mbed::Watchdog`), refrescado desde `loop()` solo si el `motorTask` reporta actividad (heartbeat). Es la única protección ante un panic, un `LV_ASSERT_HANDLER` (`while(1)`) o un cuelgue. Ojo: el IWDG sigue activo en el bootloader. Usar un timeout que no interfiera con la subida de firmware.
- [ ] **Guardar `startPos` / `endPos` (y las extensiones) en flash.** Hoy, tras un reinicio, `isSpoolCalibrated()` es falso y el Spooler no arranca mientras el Puller sigue tirando. Va de la mano con el watchdog.
- [ ] **Detección de calibre sin datos**: si `measurement.lastUpdateTime` lleva más de X s sin actualizarse, el Puller sigue con el último valor indefinidamente. Alarma con el buzzer o velocidad fija de seguridad.
- [ ] **Distancia máxima del tensor (300 mm)**: si el bucle de filamento puede superarla, un exceso de holgura se confunde con un sensor caído y el Spooler se para. Revisar según la mecánica real.

### Ajustes (no son fallos)

- [ ] **PID:** QuickPID usa por defecto `sampleTimeUs = 100000`, pero el Puller se ejecuta cada 80 ms, así que en la práctica calcula cada 160 ms. Opciones: `SetSampleTimeUs(interval * 1000)` o modo `Control::timer`. Cambia la dinámica: requiere volver a sintonizar.
- [ ] **Escrituras en flash**: los spinbox guardan en cada `LV_EVENT_LONG_PRESSED_REPEAT`. Guardar solo al soltar (`LV_EVENT_RELEASED`) para reducir el desgaste.
- [ ] Valorar `LV_MEM_SIZE` (64 KB) si la UI crece: un fallo de memoria de LVGL acaba en `while(1)`.

### Hardware (probable origen del ruido SPI)

- [ ] **VCC_IO de cada driver:** condensador de 100 nF + 10 µF entre VIO y GND, pegado al módulo. El umbral de reset llega a 3,0 V y la alimentación es de 3,3 V.
- [ ] Pull-up de 10 kΩ a 3,3 V en cada línea CS (pines 8, 9, 10). Mientras el Giga arranca o se reinicia, CS queda al aire y el chip puede ejecutar basura.
- [ ] Confirmar que VIO de los TMC5160 es 3,3 V: los pines del Giga no admiten 5 V.
- [ ] Resistencia serie de 33–100 Ω en SCK, junto al Giga, si el cable mide más de unos 15 cm: amortigua el rebote que el chip ve como un pulso de reloj extra.
- [ ] Cables SPI cortos, apantallados o trenzados, separados de las fases de los motores.
- [ ] Condensador electrolítico de bajo ESR en VM, junto a cada driver: el datasheet pide al menos 100 µF por amperio de corriente de bobina (3.1). Las caídas de VM provocan resets reales del chip (`GSTAT.reset` / `uv_cp`).
- [ ] Masa común sólida entre la alimentación de motores y el Giga.
