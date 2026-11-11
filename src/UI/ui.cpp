#include "UI/ui.h"
#include "UI/ui_kit.h"
#include "Commons/globals.h"
#include "Commons/EventLog.h"
#include <math.h>

// Propuesta B: riel lateral de 96 px + cabecera de estado fija de 64 px + página de 704x416.
// Las fuentes Montserrat de LVGL solo traen ASCII: los textos van sin tildes.
// ponytail: tildes y símbolos (±, σ, ·) requieren una fuente propia generada con lv_font_conv.

#define TOL 0.07f        // tolerancia aceptada: setpoint ± 0,07 mm (en tolerancia / fuera)
#define RANGE 0.20f      // rango fijo de la gráfica: setpoint ± 0,20 (el setpoint queda en el centro)
#define HIST 150         // 15 s de historial de diámetro, una muestra cada 100 ms
#define CORE_D 100.0f    // mm, bobina típica de 1 kg (solo para dibujar el llenado)
#define FLANGE_D 200.0f
#define MAX_SPEED_UI 18000.0f  // mismo tope que la UI anterior
#define MAX_WEIGHT_UI 5000.0f

void ui_tick(lv_timer_t*);

// ---------------------------------------------------------------------------
// Datos derivados
// ---------------------------------------------------------------------------
// Unidades del puller: 800 micropasos por unidad y unidades/min -> 64 unidades por vuelta
static float lineSpeed() { return puller.speed / 64.0f * PI * PULLER_DIAM / 1000.0f; } // m/min
// 1 m de filamento = sección (mm²) en cm³; misma base que getExtrudedFilamentWeight()
static float gramsPerMin() { return lineSpeed() * PI * diameter * diameter / 4.0f * plasticTypes[selectedPlasticTypeIndex].density; }
static bool  inTol(float v) { return fabsf(v - diameter) <= TOL + 1e-4f; }
static bool  tensionerLost() { return tensioner.missedEchoes >= TENSIONER_MAX_MISSED_ECHOES; }
static bool  spoolFull(float w) { return alertsEnabled && weightAlertThreshold > 0 && w >= weightAlertThreshold; }
static bool  pullerSaturated() { return puller.speed >= puller.maxOutput || puller.speed <= puller.minOutput; }
static bool  alignerBusy() {
  AlignerState s = aligner.getCurrentState();
  return aligner.isHoming() || s == SPOOL_CALIBRATION || s == WAITING;
}
static float mmOf(int32_t steps) { return steps / (float)STEPS_PER_CM * 10.0f; }
static float spoolFill(float lengthM) {
  float width = mmOf(aligner.getEffectiveEndPos() - aligner.getEffectiveStartPos());
  float area = PI * diameter * diameter / 4.0f;
  float d = sqrtf(CORE_D * CORE_D + 4.0f * lengthM * 1000.0f * area / (PI * fmaxf(width, 10.0f) * 0.8f));
  return fminf(fmaxf((d - CORE_D) / (FLANGE_D - CORE_D), 0.0f), 1.0f);
}
static float posPct() {
  int32_t s = aligner.getEffectiveStartPos(), e = aligner.getEffectiveEndPos();
  return e > s ? fminf(fmaxf((aligner.currentPosition() - s) / (float)(e - s), 0.0f), 1.0f) : 0.0f;
}
// Estado general del bobinado, en palabras del operario
static const char* status(lv_color_t* color, float weight) {
  if (tensionerLost())                              { *color = C_RED;    return "Sin lectura del tensor"; }
  if (spoolFull(weight))                            { *color = C_YELLOW; return "Bobina llena"; }
  if (aligner.isHoming())                           { *color = lv_color_hex(0x6FB7E8); return "Buscando origen"; }
  if (aligner.getCurrentState() == SPOOL_CALIBRATION) { *color = lv_color_hex(0x6FB7E8); return "Calibrando bobina"; }
  if (!aligner.isSpoolCalibrated())                 { *color = C_YELLOW; return "Bobina sin calibrar"; }
  *color = C_GREEN;
  return "Bobinando";
}

// Historial de diámetro (gráfica, reparto y desviación)
static float hist[HIST];
static int histN = 0, histHead = 0;
static void histPush(float v) { hist[histHead] = v; histHead = (histHead + 1) % HIST; if (histN < HIST) histN++; }
static void histStats(float* mean, float* sd, float* okPct) {
  if (!histN) { *mean = diameter; *sd = 0; *okPct = 1; return; }
  float m = 0; int ok = 0;
  for (int i = 0; i < histN; i++) { m += hist[i]; ok += inTol(hist[i]); }
  m /= histN;
  float s = 0;
  for (int i = 0; i < histN; i++) s += (hist[i] - m) * (hist[i] - m);
  *mean = m; *sd = sqrtf(s / histN); *okPct = ok / (float)histN;
}

// Alertas: copia del registro persistente (Commons/EventLog), más reciente primero
static LogEntry alerts[LOG_MAX];
static int nAlerts = 0;      // todas, incluidas las de arranques anteriores
static int nNew = 0;         // de este arranque, sin contar el propio arranque (globo del riel)
static uint32_t logVer = ~0u;
static void syncAlerts() {
  uint32_t v = eventLogVersion();
  if (v == logVer) return;
  logVer = v;
  nAlerts = eventLogSnapshot(alerts);
  nNew = 0;
  for (int i = 0; i < nAlerts; i++) nNew += alerts[i].boot == eventLogBoot() && alerts[i].kind != EV_BOOT;
}

static const char* regName(uint32_t addr) {
  switch (addr) {
    case 0x00: return "GCONF";
    case 0x20: return "RAMPMODE";
    case 0x34: return "SW_MODE";
    case 0x6C: return "CHOPCONF";
    default:   return "registro";
  }
}
static const char* resetName(uint32_t r) {
  switch (r) {
    case RESET_REASON_POWER_ON:  return "encendido";
    case RESET_REASON_PIN_RESET: return "boton de reset";
    case RESET_REASON_BROWN_OUT: return "caida de tension";
    case RESET_REASON_SOFTWARE:  return "software";
    case RESET_REASON_WATCHDOG:  return "watchdog";
    case RESET_REASON_LOCKUP:    return "bloqueo del procesador";
    case RESET_REASON_MULTIPLE:  return "varias causas";
    default:                     return "otra";
  }
}
// Texto de una entrada: línea principal y detalle con lo leído del driver. Devuelve si es un fallo grave.
static bool alertText(const LogEntry& e, char* msg, char* det, size_t n) {
  static const char* const srcName[SRC_SYS + 1] = { "Puller", "Alineador", "Bobina", "Sistema" };
  const char* s = srcName[e.src];
  unsigned c = e.count;
  unsigned long a = e.a, b = e.b, cc = e.c;
  det[0] = 0;
  switch (e.kind) {
    case EV_BOOT:
      lv_snprintf(msg, n, "Arranque n. %u", (unsigned)e.boot);
      lv_snprintf(det, n, "Causa: %s (0x%08lX)", resetName(e.a), b);
      return false;
    case EV_TMC_GLITCH:
      lv_snprintf(msg, n, "%s: lectura SPI corrupta (x%u)", s, c);
      lv_snprintf(det, n, "GSTAT=0x%08lX  IOIN=0x%08lX", a, b);
      return false;
    case EV_TMC_STATUS:
      lv_snprintf(msg, n, "%s: bit de reset espurio (x%u)", s, c);
      lv_snprintf(det, n, "Config reescrita sin perder la posicion");
      return false;
    case EV_TMC_REPAIR:
      lv_snprintf(msg, n, "%s: config corregida (x%u)", s, c);
      lv_snprintf(det, n, "%s escrito 0x%08lX, leido 0x%08lX", regName(e.a), b, cc);
      return false;
    case EV_TMC_LOST:
      lv_snprintf(msg, n, "%s: reinicio por baja tension (x%u)", s, c);
      lv_snprintf(det, n, "GSTAT=0x%08lX  CHOPCONF=0x%08lX", a, b);
      return true;
    case EV_TMC_DRVERR: {
      // Bits de DRV_STATUS (datasheet 6.5.5)
      static const struct { uint8_t bit; const char* name; } flags[] = {
        { 25, "sobretemp." }, { 26, "prealarma temp." }, { 27, "corto masa A" },
        { 28, "corto masa B" }, { 12, "corto VS A" }, { 13, "corto VS B" },
      };
      char f[48] = "";
      for (auto& fl : flags) {
        if (!((e.b >> fl.bit) & 1)) continue;
        if (f[0]) strncat(f, ", ", sizeof(f) - strlen(f) - 1);
        strncat(f, fl.name, sizeof(f) - strlen(f) - 1);
      }
      lv_snprintf(msg, n, "%s: apagado, temp. o corto (x%u)", s, c);
      lv_snprintf(det, n, "%s  DRV_STATUS=0x%08lX", f[0] ? f : "sin flag", b);
      return true;
    }
    case EV_TMC_UVCP:
      lv_snprintf(msg, n, "%s: baja tension en motores (x%u)", s, c);
      lv_snprintf(det, n, "GSTAT=0x%08lX", a);
      return true;
    case EV_TENSIONER_LOST:
      lv_snprintf(msg, n, "Tensor: el sensor no devuelve eco (x%u)", c);
      return true;
    case EV_SPOOL_FULL:
      lv_snprintf(msg, n, "Bobina llena: %lu g alcanzados", a);
      return false;
    case EV_UI_HANG:
      lv_snprintf(msg, n, "Pantalla sin responder %lu s (x%u)", a / 1000, c);
      lv_snprintf(det, n, "Tambien se para la lectura del calibre");
      return true;
    case EV_MOTOR_HANG:
      lv_snprintf(msg, n, "Control de motores sin responder %lu s (x%u)", a / 1000, c);
      lv_snprintf(det, n, "Los drivers siguen a la ultima velocidad");
      return true;
    default:
      lv_snprintf(msg, n, "Evento %u", (unsigned)e.kind);
      return false;
  }
}

// ---------------------------------------------------------------------------
// Acciones
// ---------------------------------------------------------------------------
static float pDia, pMin, pMax, pAlert;  // valores pendientes mientras se mantiene un ±
static void commitDia(float v)   { updateDiameter(roundf(v * 100) / 100); pDia = diameter; }
static void commitMin(float v)   { puller.updateMinOutput((uint32_t)fminf(v, (float)puller.maxOutput)); pMin = puller.minOutput; uiSheetRefresh(); }
static void commitMax(float v)   { puller.updateMaxOutput((uint32_t)fmaxf(v, (float)puller.minOutput)); pMax = puller.maxOutput; uiSheetRefresh(); }
static void commitAlert(float v) { updateWeightAlertThreshold((int16_t)v); pAlert = weightAlertThreshold; uiSheetRefresh(); }

static void doNewSpool() { resetMeasurements(); uiToast("Contadores a cero"); }
static void newSpool_cb(lv_event_t*) {
  uiConfirm("Empezar una bobina nueva?", "Pone a cero longitud y peso, reinicia min/max y silencia el aviso.", "Nueva bobina", doNewSpool);
}
static void doResetMM() { resetMinMax(); uiToast("Min/max reiniciados"); }
static void resetMM_cb(lv_event_t*) {
  uiConfirm("Reiniciar min/max del calibre?", "Descarta las lecturas anteriores, por ejemplo las del enhebrado. No toca el peso.", "Reiniciar", doResetMM);
}
static void doHome() { aligner.requestResetHome(); uiToast("Buscando origen"); }
static void doCalib() { aligner.requestStartCalibration(); uiToast("Calibrando bobina"); }
static void home_cb(lv_event_t*) { uiConfirm("Buscar el origen del alineador?", "Va hasta el tope y vuelve al inicio de la bobina.", "Continuar", doHome); }
static void calib_cb(lv_event_t*) { uiConfirm("Recalibrar la bobina?", "El alineador busca las dos valonas. Las extensiones vuelven a 0.", "Calibrar", doCalib); }

static void move_cb(lv_event_t* e) {
  int dir = (int)(intptr_t)lv_event_get_user_data(e);
  volatile bool& flag = dir < 0 ? aligner.canMoveLeft : aligner.canMoveRight;
  switch (lv_event_get_code(e)) {
    case LV_EVENT_LONG_PRESSED: flag = true; break;                                       // mantener: continuo
    case LV_EVENT_RELEASED: case LV_EVENT_PRESS_LOST: flag = false; break;
    case LV_EVENT_SHORT_CLICKED: aligner.requestManualMove(dir * aligner_to_move); break;  // toque: un paso
    default: break;
  }
}
static void ext_cb(lv_event_t* e) {
  if (!aligner.isSpoolCalibrated()) { uiToast("Calibra la bobina antes de ajustar los limites", true); return; }
  switch ((int)(intptr_t)lv_event_get_user_data(e)) {
    case 0: aligner.removeStartExtension(); break;
    case 1: aligner.addStartExtension(); break;
    case 2: aligner.removeEndExtension(); break;
    case 3: aligner.addEndExtension(); break;
  }
}
static void diaKey_cb(lv_event_t*) { uiKeypad("Setpoint del diametro", "mm", DIAMETER_MIN, DIAMETER_MAX, 2, commitDia); }
static void minKey_cb(lv_event_t*) { uiKeypad("Traccion minima", "pasos/s", 0, puller.maxOutput, 0, commitMin); }
static void maxKey_cb(lv_event_t*) { uiKeypad("Traccion maxima", "pasos/s", puller.minOutput, MAX_SPEED_UI, 0, commitMax); }

static UiStepper stDiaDn = { &pDia, -0.01f, DIAMETER_MIN, DIAMETER_MAX, commitDia }, stDiaUp = { &pDia, 0.01f, DIAMETER_MIN, DIAMETER_MAX, commitDia };
static UiStepper stMinDn = { &pMin, -50, 0, MAX_SPEED_UI, commitMin }, stMinUp = { &pMin, 50, 0, MAX_SPEED_UI, commitMin };
static UiStepper stMaxDn = { &pMax, -50, 0, MAX_SPEED_UI, commitMax }, stMaxUp = { &pMax, 50, 0, MAX_SPEED_UI, commitMax };

// ---------------------------------------------------------------------------
// Piezas comunes
// ---------------------------------------------------------------------------
static lv_obj_t* card(lv_obj_t* p, int x, int y, int w, int h) { return uiBox(p, x, y, w, h, C_CARD, 12); }
static lv_obj_t* at(lv_obj_t* o, int x, int y) { lv_obj_set_pos(o, x, y); return o; }
static lv_obj_t* caps(lv_obj_t* p, const char* t, int x, int y) {
  lv_obj_t* l = uiLabel(p, &lv_font_montserrat_16, C_MUTED, t);
  lv_obj_set_style_text_letter_space(l, 1, 0);
  return at(l, x, y);
}
static lv_obj_t* btnAt(lv_obj_t* p, int x, int y, int w, int h, lv_color_t bg = C_BG) {
  lv_obj_t* b = uiBtn(p, bg);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_size(b, w, h);
  return b;
}
static lv_obj_t* valueBtn(lv_obj_t* p, int x, int y, int w, int h, lv_obj_t** lbl, const lv_font_t* f, lv_event_cb_t cb) {
  lv_obj_t* b = btnAt(p, x, y, w, h, C_DARK);
  lv_obj_set_style_border_color(b, C_GRID, 0);
  lv_obj_set_style_border_width(b, 2, 0);
  *lbl = uiLabel(b, f, C_TEXT);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
  return b;
}
static lv_obj_t* stepBtn(lv_obj_t* p, int x, int y, int w, int h, bool plus, UiStepper* s) {
  lv_obj_t* b = btnAt(p, x, y, w, h);
  uiLabel(b, &lv_font_montserrat_24, C_TEXT, plus ? LV_SYMBOL_PLUS : LV_SYMBOL_MINUS);
  uiStepper(b, s);
  return b;
}
// Chip de estado: punto + texto sobre fondo tenue del mismo color
static lv_obj_t* chip(lv_obj_t* p, int x, int y) {
  lv_obj_t* c = lv_obj_create(p);
  lv_obj_remove_style_all(c);
  lv_obj_set_pos(c, x, y);
  lv_obj_set_size(c, LV_SIZE_CONTENT, 30);
  lv_obj_set_style_radius(c, 15, 0);
  lv_obj_set_style_bg_opa(c, LV_OPA_20, 0);
  lv_obj_set_style_pad_hor(c, 12, 0);
  lv_obj_set_style_pad_gap(c, 8, 0);
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);
  uiBox(c, 0, 0, 10, 10, C_GREEN, LV_RADIUS_CIRCLE);
  uiLabel(c, &lv_font_montserrat_16, C_GREEN);
  return c;
}
static void chipSet(lv_obj_t* c, lv_color_t col, const char* text) {
  lv_obj_set_style_bg_color(c, col, 0);
  lv_obj_set_style_bg_color(lv_obj_get_child(c, 0), col, 0);
  lv_obj_set_style_text_color(lv_obj_get_child(c, 1), col, 0);
  uiSetText(lv_obj_get_child(c, 1), "%s", text);
}
static lv_obj_t* dashed(lv_obj_t* p, lv_color_t c, int w) {
  lv_obj_t* l = lv_line_create(p);
  lv_obj_set_style_line_color(l, c, 0);
  lv_obj_set_style_line_width(l, w, 0);
  lv_obj_set_style_line_dash_width(l, 5, 0);
  lv_obj_set_style_line_dash_gap(l, 4, 0);
  return l;
}
// Iconos dibujados con primitivas: la fuente "icons" solo existe a 48 px y no tiene bobina ni puller.
// Las piezas con user_data = 1 toman el color del icono; el resto son huecos en el color de fondo.
enum IconKind { IC_DASH, IC_SPOOL, IC_PULLER };
static lv_obj_t* part(lv_obj_t* ic, float s, float x, float y, float w, float h, int r, bool tint, lv_color_t col) {
  lv_obj_t* o = uiBox(ic, (int)(x * s + 0.5f), (int)(y * s + 0.5f), max(1, (int)(w * s + 0.5f)), max(1, (int)(h * s + 0.5f)), col, r);
  if (tint) lv_obj_set_user_data(o, (void*)1);
  return o;
}
static void iconColor(lv_obj_t* ic, lv_color_t col) {
  if (lv_obj_check_type(ic, &lv_label_class)) { lv_obj_set_style_text_color(ic, col, 0); return; }
  for (uint32_t i = 0; i < lv_obj_get_child_count(ic); i++) {
    lv_obj_t* o = lv_obj_get_child(ic, i);
    if (lv_obj_get_user_data(o)) lv_obj_set_style_bg_color(o, col, 0);
  }
}
// Icono de 40x40 unidades escalado a "size" px
static lv_obj_t* drawIcon(lv_obj_t* parent, IconKind k, int size, lv_color_t col, lv_color_t hole) {
  lv_obj_t* ic = uiBox(parent, 0, 0, size, size, hole);
  lv_obj_set_style_bg_opa(ic, LV_OPA_TRANSP, 0);
  float s = size / 40.0f;
  if (k == IC_DASH) {           // gráfica de barras
    const float h[4] = { 16, 28, 20, 38 };
    for (int i = 0; i < 4; i++) part(ic, s, 1 + i * 10.5f, 40 - h[i], 7, h[i], 2, true, col);
  } else if (k == IC_SPOOL) {   // bobina: dos valonas y el filamento enrollado
    part(ic, s, 1, 1, 7, 38, 3, true, col);
    part(ic, s, 32, 1, 7, 38, 3, true, col);
    part(ic, s, 10, 9, 20, 22, 2, true, col);
    part(ic, s, 10, 15.5f, 20, 2, 0, false, hole);
    part(ic, s, 10, 22.5f, 20, 2, 0, false, hole);
  } else {                      // puller: dos rodillos pinzando el filamento
    part(ic, s, 0, 18.5f, 40, 3, 1, true, col);
    part(ic, s, 11, 0, 18, 18, LV_RADIUS_CIRCLE, true, col);
    part(ic, s, 11, 22, 18, 18, LV_RADIUS_CIRCLE, true, col);
    part(ic, s, 17, 6, 6, 6, LV_RADIUS_CIRCLE, false, hole);
    part(ic, s, 17, 28, 6, 6, LV_RADIUS_CIRCLE, false, hole);
  }
  return ic;
}
static void setPressed(lv_obj_t* b, bool on) { lv_obj_set_style_bg_color(b, on ? C_ORANGE : C_BG, 0); }

// ---------------------------------------------------------------------------
// Riel y cabecera
// ---------------------------------------------------------------------------
#define NPAGES 4  // Dashboard, Bobina, Alertas, Ajustes
static lv_obj_t* pages[NPAGES];
static lv_obj_t* tabs[NPAGES];
static int page = 0;
static lv_obj_t *hState, *hDia, *hSpeed, *hWeight, *badge;

// Riel: botones de 76x76 separados 28 px; la pestaña activa lleva una barra naranja en el borde
#define TAB_SIZE 76
#define TAB_STEP (TAB_SIZE + 28)
#define TAB_TOP 16
static lv_obj_t* railMark;

static void showPage(int p) {
  page = p;
  for (int i = 0; i < NPAGES; i++) {
    bool on = i == p;
    if (on) lv_obj_remove_flag(pages[i], LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(tabs[i], on ? C_BG : C_DARK, 0);
    iconColor(lv_obj_get_child(tabs[i], 0), on ? C_ORANGE : C_MUTED);
  }
  lv_obj_set_y(railMark, TAB_TOP + p * TAB_STEP + (TAB_SIZE - 44) / 2);
  ui_tick(nullptr);
}
static void tab_cb(lv_event_t* e) { showPage((int)(intptr_t)lv_event_get_user_data(e)); }

static void buildRail(lv_obj_t* scr) {
  uiBox(scr, 0, 0, 96, 480, C_DARK);
  for (int i = 0; i < NPAGES; i++) {
    lv_obj_t* b = uiBtn(scr, C_DARK, C_MUTED, true);
    lv_obj_set_pos(b, (96 - TAB_SIZE) / 2, TAB_TOP + i * TAB_STEP);
    lv_obj_set_size(b, TAB_SIZE, TAB_SIZE);
    lv_obj_set_style_radius(b, 14, 0);
    if (i < 2) drawIcon(b, i == 0 ? IC_DASH : IC_SPOOL, 40, C_MUTED, C_DARK);
    else uiLabel(b, &lv_font_montserrat_40, C_MUTED, i == 2 ? LV_SYMBOL_BELL : LV_SYMBOL_SETTINGS);
    lv_obj_add_event_cb(b, tab_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    tabs[i] = b;
  }
  // Globo con el número de alertas sobre la pestaña Alertas
  badge = uiBox(tabs[2], 0, 0, 28, 24, C_RED, 12);
  lv_obj_add_flag(badge, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_set_pos(badge, TAB_SIZE - 32, 4);
  lv_obj_center(uiLabel(badge, &lv_font_montserrat_16, lv_color_white(), "0"));
  railMark = uiBox(scr, 0, 0, 4, 44, C_ORANGE, 2);
}

static lv_obj_t* hdrCell(lv_obj_t* hdr, int w) {
  lv_obj_t* c = lv_obj_create(hdr);
  lv_obj_remove_style_all(c);
  lv_obj_set_size(c, w, 64);
  lv_obj_set_style_border_side(c, LV_BORDER_SIDE_LEFT, 0);
  lv_obj_set_style_border_color(c, C_GRID, 0);
  lv_obj_set_style_border_width(c, 1, 0);
  lv_obj_set_style_pad_hor(c, 14, 0);
  lv_obj_set_style_pad_gap(c, 8, 0);
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(c, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  return c;
}
static void buildHeader(lv_obj_t* scr) {
  lv_obj_t* hdr = uiBox(scr, 96, 0, 704, 64, C_BG);
  lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_color(hdr, C_GRID, 0);
  lv_obj_set_style_border_width(hdr, 1, 0);
  lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
  lv_obj_t* st = hdrCell(hdr, 10);
  lv_obj_set_flex_grow(st, 1);
  lv_obj_set_style_border_width(st, 0, 0);
  uiBox(st, 0, 0, 14, 14, C_GREEN, LV_RADIUS_CIRCLE);
  hState = uiLabel(st, &lv_font_montserrat_18, C_TEXT);
  lv_label_set_long_mode(hState, LV_LABEL_LONG_WRAP);  // "Sin lectura del tensor" ocupa dos líneas
  lv_obj_set_flex_grow(hState, 1);
  lv_obj_t* c = hdrCell(hdr, 124);
  uiLabel(c, &lv_font_montserrat_16, C_MUTED, "D");
  hDia = uiLabel(c, &lv_font_montserrat_28, C_TEXT);
  c = hdrCell(hdr, 152);
  hSpeed = uiLabel(c, &lv_font_montserrat_28, C_TEXT);
  uiLabel(c, &lv_font_montserrat_16, C_MUTED, "g/min");
  c = hdrCell(hdr, 150);
  hWeight = uiLabel(c, &lv_font_montserrat_28, C_TEXT);
}

// ---------------------------------------------------------------------------
// Medida: gráfica con reparto lateral, estadísticas, lectura actual y consigna
// ---------------------------------------------------------------------------
#define MX0 56
#define MY0 18
#define MPW 384
#define MPH 292
static lv_obj_t *mChart, *mGrid[4], *mGridLine[4], *mBand, *mSetLine, *mMM[2], *mMMLbl[2], *mStat[4], *mNowChip, *mNow, *mNowTol, *mSet;
static lv_chart_series_t* mSer;
static lv_point_precise_t mMMPts[2][2];
// Escala vertical fija: consigna ± RANGE
static float vLo = -1, vHi = 0;
static float mY(float v) { return MY0 + (vHi - constrain(v, vLo, vHi)) / (vHi - vLo) * MPH; }

static void buildMedida(lv_obj_t* pg) {
  lv_obj_t* cc = card(pg, 0, 0, 454, 396);
  mBand = uiBox(cc, MX0, 0, MPW, 1, C_GREEN);
  lv_obj_set_style_bg_opa(mBand, LV_OPA_10, 0);
  for (int i = 0; i < 4; i++) {
    mGridLine[i] = uiBox(cc, MX0, 0, MPW, 1, C_GRID);
    mGrid[i] = uiLabel(cc, &lv_font_montserrat_16, C_MUTED);
  }
  mSetLine = uiBox(cc, MX0, 0, MPW, 2, C_ORANGE);
  lv_obj_set_style_bg_opa(mSetLine, LV_OPA_50, 0);
  mChart = lv_chart_create(cc);
  lv_obj_set_pos(mChart, MX0, MY0);
  lv_obj_set_size(mChart, MPW, MPH);
  lv_obj_set_style_bg_opa(mChart, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(mChart, 0, 0);
  lv_obj_set_style_pad_all(mChart, 0, 0);
  lv_obj_set_style_radius(mChart, 0, 0);
  lv_obj_set_style_line_width(mChart, 3, LV_PART_ITEMS);
  lv_obj_set_style_width(mChart, 0, LV_PART_INDICATOR);
  lv_obj_set_style_height(mChart, 0, LV_PART_INDICATOR);
  lv_chart_set_type(mChart, LV_CHART_TYPE_LINE);
  lv_chart_set_div_line_count(mChart, 0, 0);
  lv_chart_set_point_count(mChart, HIST);
  lv_chart_set_update_mode(mChart, LV_CHART_UPDATE_MODE_SHIFT);
  mSer = lv_chart_add_series(mChart, C_FIL, LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_all_values(mChart, mSer, LV_CHART_POINT_NONE);
  for (int i = 0; i < 2; i++) { mMM[i] = dashed(cc, C_YELLOW, 2); mMMLbl[i] = uiLabel(cc, &lv_font_montserrat_16, C_YELLOW); }
  const char* names[4] = { "MEDIA", "DESV.", "MIN", "MAX" };
  for (int i = 0; i < 4; i++) {
    lv_obj_t* s = uiBox(cc, 12 + i * 109, 322, 101, 62, C_BG, 8);
    caps(s, names[i], 10, 6);
    mStat[i] = at(uiLabel(s, &lv_font_montserrat_24, i >= 2 ? C_YELLOW : C_TEXT), 10, 28);
  }

  lv_obj_t* now = card(pg, 464, 0, 220, 128);
  mNowChip = chip(now, 12, 10);
  mNow = at(uiLabel(now, &lv_font_montserrat_48, C_TEXT), 14, 42);
  at(uiLabel(now, &lv_font_montserrat_20, C_MUTED, "mm"), 166, 66);
  mNowTol = at(uiLabel(now, &lv_font_montserrat_16, C_MUTED), 14, 100);

  lv_obj_t* cs = card(pg, 464, 138, 220, 174);
  caps(cs, "SETPOINT", 12, 10);
  lv_obj_t* sv = valueBtn(cs, 10, 36, 200, 64, &mSet, &lv_font_montserrat_28, diaKey_cb);
  uiLabel(sv, &lv_font_montserrat_16, C_MUTED, "mm");  // unidad pequeña y en gris, como en el artifact
  stepBtn(cs, 10, 108, 96, 56, false, &stDiaDn);
  stepBtn(cs, 114, 108, 96, 56, true, &stDiaUp);

  lv_obj_t* mm = btnAt(pg, 464, 322, 220, 74, C_CARD);
  lv_obj_set_style_radius(mm, 12, 0);
  // Icono a la izquierda y texto en dos líneas: «Reiniciar min/max» no cabe en una a 20 px
  lv_obj_set_flex_flow(mm, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(mm, 14, 0);
  uiLabel(mm, &lv_font_montserrat_28, C_TEXT, LV_SYMBOL_REFRESH);
  uiLabel(mm, &lv_font_montserrat_20, C_TEXT, "Reiniciar\nmin/max");
  lv_obj_add_event_cb(mm, resetMM_cb, LV_EVENT_CLICKED, nullptr);
}

static void renderMedida(float last) {
  // Escala fija centrada en la consigna; solo se recalcula si cambia la consigna
  if (vLo != diameter - RANGE) {
    vLo = diameter - RANGE; vHi = diameter + RANGE;
    lv_chart_set_range(mChart, LV_CHART_AXIS_PRIMARY_Y, (int32_t)lroundf(vLo * 100), (int32_t)lroundf(vHi * 100));
  }
  const float gv[] = { vHi, diameter + TOL, diameter - TOL, vLo };
  for (int i = 0; i < 4; i++) {
    int y = (int)mY(gv[i]);
    lv_obj_set_y(mGridLine[i], y);
    uiSetText(mGrid[i], "%.2f", gv[i]);
    lv_obj_set_pos(mGrid[i], 8, y - 9);
  }
  int yt = (int)mY(diameter + TOL), yb = (int)mY(diameter - TOL);
  lv_obj_set_y(mBand, yt);
  lv_obj_set_height(mBand, max(1, yb - yt));
  lv_obj_set_y(mSetLine, (int)mY(diameter) - 1);
  // Mín./máx. registrados: solo se dibujan si caen dentro de la escala visible
  const float mm[2] = { measurementMaxRead, measurementMinRead };
  for (int i = 0; i < 2; i++) {
    float y = mY(mm[i]);
    bool vis = mm[i] >= vLo && mm[i] <= vHi;
    mMMPts[i][0] = { MX0, y }; mMMPts[i][1] = { MX0 + MPW, y };
    lv_line_set_points(mMM[i], mMMPts[i], 2);
    uiSetText(mMMLbl[i], i ? "min %.2f" : "max %.2f", mm[i]);
    lv_obj_set_pos(mMMLbl[i], MX0 + 4, (int)(i ? y + 3 : y - 21));
    if (vis) { lv_obj_remove_flag(mMM[i], LV_OBJ_FLAG_HIDDEN); lv_obj_remove_flag(mMMLbl[i], LV_OBJ_FLAG_HIDDEN); }
    else { lv_obj_add_flag(mMM[i], LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(mMMLbl[i], LV_OBJ_FLAG_HIDDEN); }
  }
  float mean, sd, okPct; histStats(&mean, &sd, &okPct);
  uiSetText(mStat[0], "%.2f", mean);
  uiSetText(mStat[1], "%.3f", sd);
  uiSetText(mStat[2], "%.2f", measurementMinRead);
  uiSetText(mStat[3], "%.2f", measurementMaxRead);
  bool ok = inTol(last);
  chipSet(mNowChip, ok ? C_GREEN : C_RED, ok ? "En tolerancia" : "Fuera de tolerancia");
  uiSetText(mNow, "%.2f", last);
  lv_obj_set_style_text_color(mNow, ok ? C_TEXT : lv_color_hex(0xFF8A70), 0);
  uiSetText(mNowTol, "%d %% en +/-%.2f (15 s)", (int)(okPct * 100 + 0.5f), TOL);
  uiSetText(mSet, "%.2f", pDia);
}

// ---------------------------------------------------------------------------
// Bobina ("cifras y cruceta"): cifras grandes con regla del recorrido a la izquierda,
// panel de mando del alineador a la derecha (flechas grandes, origen, calibrar y límites)
// ---------------------------------------------------------------------------
#define RX0 22    // regla del recorrido: x inicial y ancho dentro de la tarjeta
#define RW 292
#define RY 166
static lv_obj_t *bChip, *bPct, *bDir, *bMm, *bMove[3], *bCal, *bExt[4], *bExtL[2];
// Regla de progreso: barra del recorrido efectivo, relleno hasta el carro, zonas de extensión,
// corchetes en las valonas, cursor (línea + triángulo bajo la barra) y extremos etiquetados
static lv_obj_t *rTrack, *rFill, *rZone[2], *rBr[2][3], *rCurLine, *rCurTri, *rName[2], *rVal[2];
static lv_point_precise_t rTriPts[32];
#define C_EXT_ADD lv_color_hex(0x6FB7E8)   // extensión que amplía el recorrido
#define C_EXT_CUT lv_color_hex(0xFF8A70)   // extensión que lo recorta
static lv_obj_t *tnDist, *tnState, *bRpm, *bFill;
static float rX(int32_t p) { return RX0 + p / (float)MAX_ALIGNER_POSITION * RW; }

static lv_obj_t* moveBtn(lv_obj_t* p, int x, int dir) {
  lv_obj_t* b = uiBtn(p, C_ORANGE, C_TEXT, true);
  lv_obj_set_pos(b, x, 40);
  lv_obj_set_size(b, 153, 120);
  uiLabel(b, &lv_font_montserrat_48, C_TEXT, dir < 0 ? LV_SYMBOL_LEFT : LV_SYMBOL_RIGHT);
  void* d = (void*)(intptr_t)dir;
  lv_obj_add_event_cb(b, move_cb, LV_EVENT_LONG_PRESSED, d);
  lv_obj_add_event_cb(b, move_cb, LV_EVENT_RELEASED, d);
  lv_obj_add_event_cb(b, move_cb, LV_EVENT_PRESS_LOST, d);
  lv_obj_add_event_cb(b, move_cb, LV_EVENT_SHORT_CLICKED, d);
  return b;
}

static void buildBobina(lv_obj_t* pg) {
  // --- Cifras ---
  lv_obj_t* l = card(pg, 0, 0, 336, 396);
  bChip = chip(l, 14, 12);
  caps(l, "POSICION", 16, 60);
  bPct = at(uiLabel(l, &lv_font_montserrat_48, C_TEXT), 16, 82);
  bDir = uiLabel(l, &lv_font_montserrat_20, C_MUTED);
  bMm = at(uiLabel(l, &lv_font_montserrat_16, C_MUTED), 16, 138);
  // Regla de progreso (barra en RY+12..RY+24, cursor debajo, etiquetas en RY+44)
  rTrack = uiBox(l, RX0, RY + 12, RW, 12, C_DARK, 3);
  rFill = uiBox(l, RX0, RY + 12, 1, 12, C_FIL, 3);
  for (int i = 0; i < 2; i++) rZone[i] = uiBox(l, 0, RY + 8, 1, 20, C_EXT_ADD);
  for (int i = 0; i < 2; i++)
    for (int k = 0; k < 3; k++) rBr[i][k] = uiBox(l, 0, 0, 1, 1, lv_color_hex(0xC9CFD5));
  rCurLine = uiBox(l, 0, RY + 8, 2, 20, C_TEXT);
  rCurTri = lv_line_create(l);  // triángulo relleno: zigzag desde el vértice a la base
  lv_obj_set_style_line_color(rCurTri, C_TEXT, 0);
  lv_obj_set_style_line_width(rCurTri, 2, 0);
  for (int g = 0; g < 2; g++) {
    rName[g] = uiLabel(l, &lv_font_montserrat_16, C_SOFT, g ? "Final" : "Inicio");
    rVal[g] = uiLabel(l, &lv_font_montserrat_16, C_MUTED);
  }
  uiBox(l, 16, 240, 304, 1, C_GRID);
  caps(l, "TENSOR", 16, 256);
  tnDist = at(uiLabel(l, &lv_font_montserrat_28, C_TEXT), 16, 280);
  tnState = at(uiLabel(l, &lv_font_montserrat_16, C_GREEN), 16, 318);
  lv_obj_t* sp = at(uiLabel(l, &lv_font_montserrat_16, C_MUTED), 16, 342);
  uiSetText(sp, "setpoint %d mm", TENSIONER_SETPOINT);
  caps(l, "BOBINA", 186, 256);
  bRpm = at(uiLabel(l, &lv_font_montserrat_28, C_TEXT), 186, 280);
  bFill = at(uiLabel(l, &lv_font_montserrat_16, C_MUTED), 186, 318);

  // --- Mandos del alineador ---
  lv_obj_t* r = card(pg, 346, 0, 338, 396);
  caps(r, "ALINEADOR", 14, 12);
  bMove[0] = moveBtn(r, 12, -1);
  bMove[2] = moveBtn(r, 173, 1);
  bMove[1] = btnAt(r, 12, 170, 153, 76);
  uiLabel(bMove[1], &lv_font_montserrat_20, C_TEXT, LV_SYMBOL_HOME "  Origen");
  lv_obj_add_event_cb(bMove[1], home_cb, LV_EVENT_CLICKED, nullptr);
  bCal = btnAt(r, 173, 170, 153, 76, C_BLUE);
  uiLabel(bCal, &lv_font_montserrat_20, C_TEXT, LV_SYMBOL_REFRESH "  Calibrar");
  lv_obj_add_event_cb(bCal, calib_cb, LV_EVENT_CLICKED, nullptr);
  for (int g = 0; g < 2; g++) {
    int y = 260 + g * 68;
    at(uiLabel(r, &lv_font_montserrat_18, C_TEXT, g ? "Final" : "Inicio"), 14, y + 6);
    bExtL[g] = at(uiLabel(r, &lv_font_montserrat_16, C_MUTED), 14, y + 30);
    for (int k = 0; k < 2; k++) {
      lv_obj_t* b = btnAt(r, 150 + k * 92, y, 84, 60);
      uiLabel(b, &lv_font_montserrat_24, C_TEXT, k ? LV_SYMBOL_PLUS : LV_SYMBOL_MINUS);
      lv_obj_add_event_cb(b, ext_cb, LV_EVENT_CLICKED, (void*)(intptr_t)(g * 2 + k));
      bExt[g * 2 + k] = b;
    }
  }
}

static void renderBobina(float len, float weight) {
  lv_color_t sc; const char* st = status(&sc, weight);
  chipSet(bChip, sc, st);
  bool busy = alignerBusy();
  int32_t es = aligner.getEffectiveStartPos(), ee = aligner.getEffectiveEndPos();
  uiSetText(bPct, "%d %%", (int)(posPct() * 100 + 0.5f));
  if (busy || aligner.getCurrentState() != AUTO_MOVE) uiSetText(bDir, "");
  else if (aligner.getCurrentDirection() == FORWARD) uiSetText(bDir, "hacia el final " LV_SYMBOL_RIGHT);
  else uiSetText(bDir, LV_SYMBOL_LEFT " hacia el inicio");
  lv_obj_align(bDir, LV_ALIGN_TOP_RIGHT, -16, 100);
  uiSetText(bMm, "%.1f de %.0f mm de recorrido", mmOf(aligner.currentPosition() - es), mmOf(ee - es));
  // Regla de progreso
  int ls = (int)rX(es), le = (int)rX(ee), gx = (int)rX(constrain(aligner.currentPosition(), es, ee));
  int vs = (int)rX(aligner.startPos), ve = (int)rX(aligner.endPos);
  lv_obj_set_x(rTrack, ls); lv_obj_set_width(rTrack, max(1, le - ls));
  lv_obj_set_x(rFill, ls); lv_obj_set_width(rFill, max(1, gx - ls));
  if (gx > ls) lv_obj_remove_flag(rFill, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(rFill, LV_OBJ_FLAG_HIDDEN);
  // Zonas de extensión entre la valona y el límite efectivo: azul amplía, rojo recorta
  const int za[2] = { min(ls, vs), min(ve, le) }, zb[2] = { max(ls, vs), max(ve, le) };
  const bool add[2] = { ls < vs, le > ve };
  for (int i = 0; i < 2; i++) {
    if (zb[i] - za[i] < 1) { lv_obj_add_flag(rZone[i], LV_OBJ_FLAG_HIDDEN); continue; }
    lv_obj_remove_flag(rZone[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_x(rZone[i], za[i]); lv_obj_set_width(rZone[i], max(3, zb[i] - za[i]));
    lv_obj_set_style_bg_color(rZone[i], add[i] ? C_EXT_ADD : C_EXT_CUT, 0);
  }
  // Corchetes en las valonas calibradas: [ al inicio, ] al final
  const int bx[2] = { vs, ve }, dir[2] = { 1, -1 };
  for (int i = 0; i < 2; i++) {
    int x = bx[i];
    lv_obj_set_pos(rBr[i][0], x - 1, RY + 4); lv_obj_set_size(rBr[i][0], 3, 28);
    int fx = dir[i] > 0 ? x - 1 : x - 6;
    lv_obj_set_pos(rBr[i][1], fx, RY + 4);  lv_obj_set_size(rBr[i][1], 8, 3);
    lv_obj_set_pos(rBr[i][2], fx, RY + 29); lv_obj_set_size(rBr[i][2], 8, 3);
  }
  // Cursor: línea fina sobre la barra y triángulo debajo (no tapa los extremos)
  lv_obj_set_x(rCurLine, gx - 1);
  int n = 0;
  for (int k = -7; k <= 7 && n < 30; k++) { rTriPts[n++] = { (float)gx, RY + 28.0f }; rTriPts[n++] = { (float)(gx + k), RY + 38.0f }; }
  lv_line_set_points(rCurTri, rTriPts, n);
  // Extremos siempre etiquetados con su extensión (gris si es 0, azul si amplía, rojo si recorta)
  int32_t ex[2] = { aligner.getStartExtensionSteps(), aligner.getEndExtensionSteps() };
  for (int g = 0; g < 2; g++) {
    if (ex[g]) uiSetText(rVal[g], "%s%.1f mm", ex[g] > 0 ? "+" : "", ex[g] * diameter); else uiSetText(rVal[g], "0 mm");
    lv_obj_set_style_text_color(rVal[g], ex[g] > 0 ? C_EXT_ADD : ex[g] < 0 ? C_EXT_CUT : C_MUTED, 0);
  }
  lv_obj_set_pos(rName[0], RX0, RY + 44);
  lv_obj_align_to(rVal[0], rName[0], LV_ALIGN_OUT_RIGHT_MID, 6, 0);
  lv_obj_update_layout(rVal[1]);
  lv_obj_set_pos(rVal[1], RX0 + RW - lv_obj_get_width(rVal[1]), RY + 44);
  lv_obj_align_to(rName[1], rVal[1], LV_ALIGN_OUT_LEFT_MID, -6, 0);

  for (int i = 0; i < 3; i++) uiEnable(bMove[i], !busy);
  uiEnable(bCal, !busy);
  for (int i = 0; i < 4; i++) uiEnable(bExt[i], aligner.isSpoolCalibrated() && !busy);
  int32_t ext[2] = { aligner.getStartExtensionSteps(), aligner.getEndExtensionSteps() };
  for (int g = 0; g < 2; g++) uiSetText(bExtL[g], "%s%.1f mm", ext[g] > 0 ? "+" : "", ext[g] * diameter);

  // Tensor: verde cerca del setpoint, amarillo lejos (> 30 mm), rojo sin eco
  bool lost = tensionerLost(), far = abs((int)tensioner.distance - TENSIONER_SETPOINT) > 30;
  if (lost) uiSetText(tnDist, "sin eco"); else uiSetText(tnDist, "%d mm", tensioner.distance);
  lv_obj_set_style_text_color(tnDist, lost ? lv_color_hex(0xFF8A70) : C_TEXT, 0);
  uiSetText(tnState, lost ? "sensor sin eco" : far ? "lejos del setpoint" : "estable");
  lv_obj_set_style_text_color(tnState, lost ? C_RED : far ? C_YELLOW : C_GREEN, 0);
  uiSetText(bRpm, "%d rpm", spooler.speed);
  uiSetText(bFill, "llenado ~%d %%", (int)(spoolFill(len) * 100 + 0.5f));
}

// ---------------------------------------------------------------------------
// Ajustes: lista de filas que abren su hoja
// ---------------------------------------------------------------------------
static lv_obj_t *aMat, *aAlert, *aPul, *aNew;
static lv_obj_t* sh[12];  // controles de la hoja abierta

static void openMat(lv_event_t*);
static void openAviso(lv_event_t*);
static void openTraccion(lv_event_t*);

static lv_obj_t* listRow(lv_obj_t* pg, int y, const char* icon, const char* title, const char* sub, lv_event_cb_t cb) {
  lv_obj_t* r = btnAt(pg, 0, y, 684, 84, C_CARD);
  lv_obj_set_layout(r, LV_LAYOUT_NONE);
  lv_obj_set_style_radius(r, 12, 0);
  lv_obj_align(uiLabel(r, &lv_font_montserrat_28, C_MUTED, icon), LV_ALIGN_LEFT_MID, 18, 0);
  at(uiLabel(r, &lv_font_montserrat_22, C_TEXT, title), 70, 14);
  at(uiLabel(r, &lv_font_montserrat_16, C_MUTED, sub), 70, 46);
  lv_obj_t* v = uiLabel(r, &lv_font_montserrat_20, C_SOFT);
  lv_obj_align(uiLabel(r, &lv_font_montserrat_20, C_MUTED, LV_SYMBOL_RIGHT), LV_ALIGN_RIGHT_MID, -18, 0);
  lv_obj_add_event_cb(r, cb, LV_EVENT_CLICKED, nullptr);
  return v;
}

static void buildAjustes(lv_obj_t* pg) {
  aMat = listRow(pg, 0, LV_SYMBOL_LIST, "Material", "Para calcular el peso de la bobina", openMat);
  aAlert = listRow(pg, 94, LV_SYMBOL_BELL, "Aviso de bobina llena", "Pita al llegar a este peso", openAviso);
  aPul = listRow(pg, 188, "", "Traccion del puller", "Velocidad minima y maxima del PID", openTraccion);
  lv_obj_t* row = lv_obj_get_parent(aPul);
  lv_obj_delete(lv_obj_get_child(row, 0));
  lv_obj_align(drawIcon(row, IC_PULLER, 30, C_MUTED, C_CARD), LV_ALIGN_LEFT_MID, 18, 0);
  aNew = listRow(pg, 282, "", "Nueva bobina", "Pone a cero longitud y peso y silencia el aviso", newSpool_cb);
  row = lv_obj_get_parent(aNew);
  lv_obj_delete(lv_obj_get_child(row, 0));
  lv_obj_align(drawIcon(row, IC_SPOOL, 30, C_MUTED, C_CARD), LV_ALIGN_LEFT_MID, 18, 0);
}

static void renderAjustes(float len, float weight, bool full) {
  uiSetText(aMat, "%s - %.2f g/cm3", plasticTypes[selectedPlasticTypeIndex].name.c_str(), plasticTypes[selectedPlasticTypeIndex].density);
  lv_obj_align(aMat, LV_ALIGN_RIGHT_MID, -52, 0);
  if (!alertsEnabled || weightAlertThreshold <= 0) uiSetText(aAlert, "Desactivado");
  else if (weightAlertThreshold < 1000) uiSetText(aAlert, "%d g", weightAlertThreshold);
  else uiSetText(aAlert, "%.2f kg", weightAlertThreshold / 1000.0f);
  lv_obj_align(aAlert, LV_ALIGN_RIGHT_MID, -52, 0);
  uiSetText(aPul, "%d - %d", puller.minOutput, puller.maxOutput);
  lv_obj_align(aPul, LV_ALIGN_RIGHT_MID, -52, 0);
  // Lo enrollado hasta ahora; «Bobina llena» en amarillo cuando salta el aviso de peso
  if (full) uiSetText(aNew, "Bobina llena");
  else if (weight < 1000) uiSetText(aNew, "%d g - %.1f m", (int)weight, len);
  else uiSetText(aNew, "%.2f kg - %.0f m", weight / 1000, len);
  lv_obj_set_style_text_color(aNew, full ? C_YELLOW : C_SOFT, 0);
  lv_obj_align(aNew, LV_ALIGN_RIGHT_MID, -52, 0);
}

// --- Hoja: material ---
static void mat_cb(lv_event_t* e) {
  updatePlasticType((uint8_t)(uintptr_t)lv_event_get_user_data(e));
  uiSheetClose();
  char m[32]; lv_snprintf(m, sizeof(m), "Material: %s", plasticTypes[selectedPlasticTypeIndex].name.c_str());
  uiToast(m);
}
static void openMat(lv_event_t*) {
  lv_obj_t* p = uiSheetOpen("Material", "Densidad para calcular el peso", nullptr);
  lv_obj_t* r = uiRow(p);
  lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW_WRAP);
  for (size_t i = 0; i < plasticTypes.size(); i++) {
    lv_obj_t* b = uiBtn(r, i == selectedPlasticTypeIndex ? C_ORANGE : C_BG, C_TEXT, true);
    lv_obj_set_size(b, 148, 76);
    uiLabel(b, &lv_font_montserrat_20, C_TEXT, plasticTypes[i].name.c_str());
    char d[16]; lv_snprintf(d, sizeof(d), "%.2f g/cm3", plasticTypes[i].density);
    uiLabel(b, &lv_font_montserrat_14, C_SOFT, d);
    lv_obj_add_event_cb(b, mat_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }
}

// --- Hoja: aviso de bobina llena ---
static UiStepper stAlDn = { &pAlert, -50, 0, MAX_WEIGHT_UI, commitAlert }, stAlUp = { &pAlert, 50, 0, MAX_WEIGHT_UI, commitAlert };
static const int presets[] = { 250, 500, 750, 1000, 2000 };
static void avisoRefresh() {
  if (alertsEnabled) lv_obj_add_state(sh[1], LV_STATE_CHECKED); else lv_obj_remove_state(sh[1], LV_STATE_CHECKED);
  if (alertsEnabled) uiSetText(sh[0], "%d g", (int)pAlert); else uiSetText(sh[0], "-");
  for (int i = 2; i <= 4; i++) uiEnable(sh[i], alertsEnabled);
  for (int i = 0; i < 5; i++) { uiEnable(sh[5 + i], alertsEnabled); setPressed(sh[5 + i], alertsEnabled && weightAlertThreshold == presets[i]); }
}
static void alSw_cb(lv_event_t* e) { updateAlertsEnabled(lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED)); avisoRefresh(); }
static void alKey_cb(lv_event_t*) { uiKeypad("Aviso de bobina llena", "g", 0, MAX_WEIGHT_UI, 0, commitAlert); }
static void alPreset_cb(lv_event_t* e) { commitAlert((float)(intptr_t)lv_event_get_user_data(e)); }
static void openAviso(lv_event_t*) {
  pAlert = weightAlertThreshold;
  lv_obj_t* p = uiSheetOpen("Aviso de bobina llena", "El zumbador suena hasta pulsar 'Nueva bobina'", avisoRefresh);
  lv_obj_t* r = uiRow(p);
  lv_obj_set_height(r, 72);
  lv_obj_set_layout(r, LV_LAYOUT_NONE);
  sh[1] = lv_switch_create(r);
  lv_obj_set_pos(sh[1], 0, 11);
  lv_obj_set_size(sh[1], 90, 50);
  lv_obj_set_style_bg_color(sh[1], C_ORANGE, LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(sh[1], C_GRID, 0);
  lv_obj_add_event_cb(sh[1], alSw_cb, LV_EVENT_VALUE_CHANGED, nullptr);
  sh[2] = stepBtn(r, 110, 0, 96, 72, false, &stAlDn);
  sh[3] = valueBtn(r, 214, 0, 448, 72, &sh[0], &lv_font_montserrat_28, alKey_cb);
  sh[4] = stepBtn(r, 670, 0, 106, 72, true, &stAlUp);
  r = uiRow(p);
  for (int i = 0; i < 5; i++) {
    char t[12]; lv_snprintf(t, sizeof(t), presets[i] < 1000 ? "%d g" : "%d kg", presets[i] < 1000 ? presets[i] : presets[i] / 1000);
    lv_obj_t* b = uiBtn(r, C_BG);
    lv_obj_set_size(b, 10, 64);
    lv_obj_set_flex_grow(b, 1);
    uiLabel(b, &lv_font_montserrat_20, C_TEXT, t);
    lv_obj_add_event_cb(b, alPreset_cb, LV_EVENT_CLICKED, (void*)(intptr_t)presets[i]);
    sh[5 + i] = b;
  }
  avisoRefresh();
}

// --- Hoja: tracción del puller ---
static void traccionRefresh() {
  const int W = 776;
  lv_obj_set_x(sh[2], (int)(pMin / MAX_SPEED_UI * W));
  lv_obj_set_width(sh[2], max(6, (int)((pMax - pMin) / MAX_SPEED_UI * W)));
  lv_obj_set_x(sh[3], (int)(constrain(puller.speed / MAX_SPEED_UI, 0.0f, 1.0f) * W) - 3);
  uiSetText(sh[0], "%d", (int)pMin);
  uiSetText(sh[1], "%d", (int)pMax);
  bool sat = pullerSaturated();
  uiSetText(sh[4], "Ahora %d pasos/s (%.1f g/min) - %s", puller.speed, gramsPerMin(), sat ? "pegado a un limite" : "corrigiendo con margen");
  lv_obj_set_style_text_color(sh[4], sat ? C_YELLOW : C_MUTED, 0);
  uiSetText(sh[5], "Velocidad de linea: %.2f m/min", lineSpeed());
}
static void openTraccion(lv_event_t*) {
  pMin = puller.minOutput; pMax = puller.maxOutput;
  lv_obj_t* p = uiSheetOpen("Traccion del puller", "Limites del PID que regula el diametro (pasos/s)", traccionRefresh);
  lv_obj_t* bar = uiRow(p);
  lv_obj_set_height(bar, 36);
  lv_obj_set_layout(bar, LV_LAYOUT_NONE);
  lv_obj_t* track = uiBox(bar, 0, 7, 776, 22, C_DARK, 11);
  sh[2] = uiBox(track, 0, 0, 10, 22, lv_color_hex(0x7A4428));
  lv_obj_set_style_border_color(sh[2], C_ORANGE, 0);
  lv_obj_set_style_border_width(sh[2], 3, 0);
  lv_obj_set_style_border_side(sh[2], (lv_border_side_t)(LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT), 0);
  sh[3] = uiBox(bar, 0, 0, 6, 36, C_TEXT, 3);
  sh[4] = uiLabel(p, &lv_font_montserrat_18, C_MUTED);
  sh[5] = uiLabel(p, &lv_font_montserrat_16, C_MUTED);
  lv_obj_set_style_margin_top(sh[5], -6, 0);  // pegado a la línea de estado
  lv_obj_t* r = uiRow(p);
  lv_obj_set_height(r, 110);
  lv_obj_set_layout(r, LV_LAYOUT_NONE);
  for (int i = 0; i < 2; i++) {
    int x = i * 392;
    at(uiLabel(r, &lv_font_montserrat_20, C_TEXT, i ? "Maximo" : "Minimo"), x, 0);
    at(uiLabel(r, &lv_font_montserrat_16, C_MUTED, i ? "nunca tira mas deprisa" : "nunca tira mas despacio"), x + 100, 3);
    stepBtn(r, x, 32, 72, 72, false, i ? &stMaxDn : &stMinDn);
    valueBtn(r, x + 80, 32, 224, 72, &sh[i], &lv_font_montserrat_28, i ? maxKey_cb : minKey_cb);
    stepBtn(r, x + 312, 32, 72, 72, true, i ? &stMaxUp : &stMinUp);
  }
  traccionRefresh();
}

// --- Página: alertas ---
static lv_obj_t *alList, *alClr;
static uint32_t shownVer = ~0u;
static uint32_t shownTens = 0;
static void doClear() { eventLogClear(); uiToast("Registro borrado"); }
static void clr_cb(lv_event_t*) {
  uiConfirm("Borrar el registro de alertas?", "Se borra tambien de la memoria flash, incluidas las de arranques anteriores.", "Borrar", doClear);
}
static void buildAlertas(lv_obj_t* pg) {
  lv_obj_t* c = card(pg, 0, 0, 684, 396);
  caps(c, "REGISTRO DE ALERTAS (FLASH)", 16, 32);
  alClr = btnAt(c, 684 - 14 - 150, 12, 150, 64);  // discreto: gris, no rojo
  uiLabel(alClr, &lv_font_montserrat_18, C_TEXT, LV_SYMBOL_TRASH "  Borrar");
  lv_obj_add_event_cb(alClr, clr_cb, LV_EVENT_CLICKED, nullptr);
  alList = lv_obj_create(c);
  lv_obj_remove_style_all(alList);
  lv_obj_set_pos(alList, 14, 88);
  lv_obj_set_size(alList, 656, 296);
  lv_obj_set_flex_flow(alList, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(alList, 8, 0);
}
// Se reconstruye al cambiar el registro y cada 10 s (para el «hace X»).
// 4 objetos por fila (32 filas como mucho) para no agotar la memoria de LVGL.
static void renderAlertas() {
  uint32_t tens = millis() / 10000;
  if (shownVer == logVer && shownTens == tens) return;
  shownVer = logVer; shownTens = tens;
  lv_obj_clean(alList);
  uiEnable(alClr, nAlerts > 0);
  if (!nAlerts) {
    lv_obj_t* e = uiRow(alList, 10);
    uiLabel(e, &lv_font_montserrat_24, C_GREEN, LV_SYMBOL_OK);
    uiLabel(e, &lv_font_montserrat_20, C_MUTED, "Sin alertas registradas");
    return;
  }
  uint16_t boot = eventLogBoot();
  for (int i = 0; i < nAlerts; i++) {
    const LogEntry& e = alerts[i];
    char msg[64], det[64];
    bool err = alertText(e, msg, det, sizeof(msg));
    lv_obj_t* row = uiBox(alList, 0, 0, 656, 60, C_BG, 8);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(row, 6, 0);
    lv_obj_set_style_border_color(row, e.kind == EV_BOOT ? C_BLUE : err ? C_RED : C_YELLOW, 0);
    // Cuándo: «hace X» si es de este arranque; si no, de qué arranque. Debajo, tiempo desde ese arranque.
    unsigned long up = e.ms / 1000;
    lv_obj_t* t = at(uiLabel(row, &lv_font_montserrat_16, C_MUTED), 18, 6);
    if (e.boot != boot) {
      uiSetText(t, "arranque %u\n+%luh%02lum", (unsigned)e.boot, up / 3600, up / 60 % 60);
    } else {
      unsigned long s = (millis() - e.ms) / 1000;
      if (s < 60) uiSetText(t, "hace %lu s\n+%luh%02lum", s, up / 3600, up / 60 % 60);
      else if (s < 3600) uiSetText(t, "hace %lu min\n+%luh%02lum", s / 60, up / 3600, up / 60 % 60);
      else uiSetText(t, "hace %lu h\n+%luh%02lum", s / 3600, up / 3600, up / 60 % 60);
    }
    at(uiLabel(row, &lv_font_montserrat_18, e.boot == boot ? C_TEXT : C_SOFT, msg), 150, 6);
    at(uiLabel(row, &lv_font_montserrat_16, C_MUTED, det), 150, 32);
  }
}

// ---------------------------------------------------------------------------
// Construcción y refresco
// ---------------------------------------------------------------------------
void build_ui() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, C_BG, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  pDia = diameter; pMin = puller.minOutput; pMax = puller.maxOutput; pAlert = weightAlertThreshold;

  buildRail(scr);
  buildHeader(scr);
  for (int i = 0; i < NPAGES; i++) {
    pages[i] = uiBox(scr, 96 + 10, 64 + 10, 684, 396, C_BG);
    lv_obj_set_style_bg_opa(pages[i], LV_OPA_TRANSP, 0);
  }
  buildMedida(pages[0]);
  buildBobina(pages[1]);
  buildAlertas(pages[2]);
  buildAjustes(pages[3]);

  lv_timer_create(ui_tick, 100, nullptr);
  showPage(0);
}

void ui_tick(lv_timer_t* timer) {
  static uint8_t tick = 0;
  static bool wasFull = false;
  if (timer) tick++;  // showPage() llama sin timer para pintar la página nueva al momento
  const float last = measurementLastRead, weight = getExtrudedFilamentWeight(), len = getExtrudedFilamentLength();
  const bool full = spoolFull(weight);

  // Los fallos de drivers, tensor y cuelgues se registran donde ocurren (Commons/EventLog);
  // aquí solo el aviso de bobina llena, que se calcula en la UI.
  if (full && !wasFull) logEvent(SRC_SYS, EV_SPOOL_FULL, weightAlertThreshold);
  wasFull = full;
  syncAlerts();

  // Historiales (se llenan aunque la página no esté visible). El calibre se registra
  // en cada refresco, sea cual sea la lectura; lo que sale de consigna ± RANGE se pinta en el borde.
  if (timer) {
    histPush(last);
    lv_chart_set_next_value(mChart, mSer, (int32_t)lroundf(constrain(last, diameter - RANGE, diameter + RANGE) * 100));
  }

  // Cabecera
  lv_color_t sc; const char* st = status(&sc, weight);
  lv_obj_set_style_bg_color(lv_obj_get_child(lv_obj_get_parent(hState), 0), sc, 0);
  uiSetText(hState, "%s", st);
  uiSetText(hDia, "%.2f", last);
  lv_obj_set_style_text_color(hDia, inTol(last) ? C_TEXT : lv_color_hex(0xFF8A70), 0);
  uiSetText(hSpeed, "%.1f", gramsPerMin());
  if (weight < 1000) uiSetText(hWeight, "%d g", (int)weight); else uiSetText(hWeight, "%.2f kg", weight / 1000);
  if (nNew) lv_obj_remove_flag(badge, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(badge, LV_OBJ_FLAG_HIDDEN);
  uiSetText(lv_obj_get_child(badge, 0), "%d", nNew);
  if (page != 2) iconColor(lv_obj_get_child(tabs[2], 0), nNew ? C_YELLOW : C_MUTED);  // campana amarilla si hay alertas de este arranque

  // Página visible
  if (page == 0) renderMedida(last);
  else if (page == 1) renderBobina(len, weight);
  else if (page == 2) renderAlertas();
  else renderAjustes(len, weight, full);

  if (timer && tick % 5 == 0) uiSheetRefresh();  // hojas abiertas a 2 Hz
}
