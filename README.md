# 🧵 ExtruderWinder

Sistema de bobinado de filamento 3D basado en Arduino GIGA R1 (STM32H747, doble core M4/M7) con interfaz táctil (LVGL) y panel web en tiempo real vía Wi‑Fi.


## ✨ Características
- 🧠 Doble core (GIGA R1):
  - M4: 🎛️ UI (LVGL), lógica de control, medición y motores (Puller, Spooler, Aligner).
  - M7: 📶 Wi‑Fi + 🌐 servidor web (SSE a 5 Hz) y 🔁 pasarela RPC.
- 🖥️ UI táctil con Arduino GIGA Display + LVGL 9.
- ⚙️ Control de motores con TMC51X0 y QuickPID.
- 🌐 Panel web ligero con actualización de estado cada 200 ms por 🔄 Server‑Sent Events.

## 📄 Licencia
Este proyecto está licenciado bajo la licencia MIT. Consulta el archivo [LICENSE](LICENSE) para más detalles.