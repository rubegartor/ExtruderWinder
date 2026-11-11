#include "UI/ui_kit.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Primitivas
// ---------------------------------------------------------------------------
lv_obj_t* uiBox(lv_obj_t* parent, int x, int y, int w, int h, lv_color_t bg, int radius) {
  lv_obj_t* o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_size(o, w, h);
  lv_obj_set_style_bg_color(o, bg, 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(o, radius, 0);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
  return o;
}

lv_obj_t* uiCircle(lv_obj_t* parent, float cx, float cy, float r, lv_color_t bg) {
  int d = (int)(r * 2 + 0.5f);
  return uiBox(parent, (int)(cx - r + 0.5f), (int)(cy - r + 0.5f), d, d, bg, LV_RADIUS_CIRCLE);
}

lv_obj_t* uiLabel(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, const char* text) {
  lv_obj_t* l = lv_label_create(parent);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, color, 0);
  lv_label_set_text(l, text);
  return l;
}

lv_obj_t* uiBtn(lv_obj_t* parent, lv_color_t bg, lv_color_t fg, bool column) {
  lv_obj_t* b = lv_obj_create(parent);
  lv_obj_remove_style_all(b);
  lv_obj_set_style_bg_color(b, bg, 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(b, lv_color_darken(bg, 40), LV_STATE_PRESSED);
  lv_obj_set_style_opa(b, LV_OPA_40, LV_STATE_DISABLED);
  lv_obj_set_style_radius(b, 10, 0);
  lv_obj_set_style_text_color(b, fg, 0);
  lv_obj_set_style_pad_hor(b, 8, 0);
  lv_obj_set_style_pad_gap(b, column ? 4 : 8, 0);
  lv_obj_set_flex_flow(b, column ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
  return b;
}

lv_obj_t* uiRow(lv_obj_t* parent, int gap) {
  lv_obj_t* r = lv_obj_create(parent);
  lv_obj_remove_style_all(r);
  lv_obj_set_size(r, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(r, gap, 0);
  lv_obj_remove_flag(r, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(r, LV_OBJ_FLAG_CLICKABLE);
  return r;
}

// Solo toca el label si el texto cambia: evita redibujar cada 100 ms
void uiSetText(lv_obj_t* label, const char* fmt, ...) {
  char buf[96];
  va_list args;
  va_start(args, fmt);
  lv_vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  if (strcmp(lv_label_get_text(label), buf) != 0) lv_label_set_text(label, buf);
}

void uiEnable(lv_obj_t* obj, bool enabled) {
  if (enabled == !lv_obj_has_state(obj, LV_STATE_DISABLED)) return;
  if (enabled) lv_obj_remove_state(obj, LV_STATE_DISABLED);
  else lv_obj_add_state(obj, LV_STATE_DISABLED);
}

// ---------------------------------------------------------------------------
// Stepper: SHORT_CLICKED y LONG_PRESSED_REPEAT cambian el valor pendiente;
// CLICKED (al soltar, también tras mantener) lo confirma una sola vez.
// Así no se escribe en la flash cada 100 ms mientras se mantiene pulsado.
// ---------------------------------------------------------------------------
static void stepper_cb(lv_event_t* e) {
  UiStepper* s = (UiStepper*)lv_event_get_user_data(e);
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    float v = *s->value + s->delta;
    *s->value = v < s->lo ? s->lo : (v > s->hi ? s->hi : v);
    uiSheetRefresh();
  } else if (code == LV_EVENT_CLICKED && s->commit) {
    s->commit(*s->value);
  }
}

void uiStepper(lv_obj_t* btn, UiStepper* cfg) {
  lv_obj_add_event_cb(btn, stepper_cb, LV_EVENT_SHORT_CLICKED, cfg);
  lv_obj_add_event_cb(btn, stepper_cb, LV_EVENT_LONG_PRESSED_REPEAT, cfg);
  lv_obj_add_event_cb(btn, stepper_cb, LV_EVENT_CLICKED, cfg);
}

// ---------------------------------------------------------------------------
// Capa modal común
// ---------------------------------------------------------------------------
static lv_obj_t* makeScrim(lv_opa_t opa) {
  lv_obj_t* s = uiBox(lv_layer_top(), 0, 0, 800, 480, lv_color_hex(0x08090C));
  lv_obj_set_style_bg_opa(s, opa, 0);
  lv_obj_add_flag(s, LV_OBJ_FLAG_CLICKABLE);
  return s;
}

// ---------------------------------------------------------------------------
// Aviso (píldora arriba al centro, 1,8 s)
// ---------------------------------------------------------------------------
void uiToast(const char* msg, bool warn) {
  const lv_color_t tone = warn ? C_YELLOW : C_GREEN, bg = lv_color_hex(0x1F262E);
  lv_obj_t* t = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(t);
  lv_obj_set_size(t, LV_SIZE_CONTENT, 52);
  lv_obj_set_style_bg_color(t, bg, 0);
  lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(t, 26, 0);
  lv_obj_set_style_clip_corner(t, true, 0);
  lv_obj_set_style_pad_left(t, 9, 0);
  lv_obj_set_style_pad_right(t, 20, 0);
  lv_obj_set_flex_flow(t, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(t, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(t, 12, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 14);
  // Icono en un círculo del color del aviso
  lv_obj_t* ci = uiBox(t, 0, 0, 34, 34, tone, LV_RADIUS_CIRCLE);
  lv_obj_center(uiLabel(ci, &lv_font_montserrat_18, bg, warn ? LV_SYMBOL_WARNING : LV_SYMBOL_OK));
  uiLabel(t, &lv_font_montserrat_16, C_TEXT, msg);
  // Barra inferior que se vacía en 1,8 s
  lv_obj_update_layout(t);
  int32_t w = lv_obj_get_width(t);
  lv_obj_t* bar = uiBox(t, 0, 0, w, 3, tone);
  lv_obj_add_flag(bar, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_set_pos(bar, -9, 49);  // relativo al contenido: compensa el padding izquierdo
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, bar);
  lv_anim_set_values(&a, w, 0);
  lv_anim_set_duration(&a, 1800);
  lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_width);
  lv_anim_start(&a);
  lv_obj_delete_delayed(t, 1800);
}

// ---------------------------------------------------------------------------
// Confirmación (sustituye a la pestaña oculta popupTab)
// ---------------------------------------------------------------------------
static VoidCb confirmOk = nullptr;
static lv_obj_t* confirmScrim = nullptr;

static void confirm_cb(lv_event_t* e) {
  bool yes = (bool)(uintptr_t)lv_event_get_user_data(e);
  VoidCb f = confirmOk;
  confirmOk = nullptr;
  lv_obj_delete_async(confirmScrim);
  confirmScrim = nullptr;
  if (yes && f) f();
}

void uiConfirm(const char* msg, const char* sub, const char* yes, VoidCb ok) {
  confirmOk = ok;
  confirmScrim = makeScrim(LV_OPA_70);
  lv_obj_t* d = uiBox(confirmScrim, 50, 60, 700, 360, C_CARD, 18);
  lv_obj_set_style_pad_all(d, 28, 0);
  lv_obj_set_pos(uiLabel(d, &lv_font_montserrat_48, C_YELLOW, LV_SYMBOL_WARNING), 0, 0);
  // Título y explicación en columna: el título puede ocupar dos líneas sin pisar la explicación
  lv_obj_t* txt = lv_obj_create(d);
  lv_obj_remove_style_all(txt);
  lv_obj_set_pos(txt, 72, 2);
  lv_obj_set_size(txt, 572, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(txt, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(txt, 12, 0);
  lv_obj_remove_flag(txt, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t* m = uiLabel(txt, &lv_font_montserrat_32, C_TEXT, msg);
  lv_label_set_long_mode(m, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(m, 572);
  lv_obj_t* s = uiLabel(txt, &lv_font_montserrat_22, C_MUTED, sub);
  lv_label_set_long_mode(s, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(s, 572);
  lv_obj_t* no = uiBtn(d, C_BG);
  lv_obj_set_size(no, 310, 96);
  lv_obj_align(no, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  uiLabel(no, &lv_font_montserrat_28, C_TEXT, LV_SYMBOL_CLOSE "  Cancelar");
  lv_obj_add_event_cb(no, confirm_cb, LV_EVENT_CLICKED, (void*)0);
  lv_obj_t* ys = uiBtn(d, C_GREEN, C_DARK);
  lv_obj_set_size(ys, 310, 96);
  lv_obj_align(ys, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  uiLabel(ys, &lv_font_montserrat_28, C_DARK, LV_SYMBOL_OK);
  uiLabel(ys, &lv_font_montserrat_28, C_DARK, yes);
  lv_obj_add_event_cb(ys, confirm_cb, LV_EVENT_CLICKED, (void*)1);
}

// ---------------------------------------------------------------------------
// Teclado numérico (admite decimales para el diámetro)
// ---------------------------------------------------------------------------
// Teclas en el orden de la rejilla de 3 columnas; OK ocupa la última fila entera
static const char* kpKeys[] = { "7", "8", "9", "4", "5", "6", "1", "2", "3", ".", "0", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK };
static struct { char t[8]; float lo, hi; uint8_t dec; NumCb ok; lv_obj_t *scrim, *val, *unit, *hint; const char* unitTxt; } kp;

static void kpDraw() {
  lv_label_set_text(kp.val, kp.t[0] ? kp.t : "0");
  lv_obj_update_layout(kp.val);
  lv_obj_align_to(kp.val, kp.unit, LV_ALIGN_OUT_LEFT_BOTTOM, -6, 6);  // valor justo a la izquierda de la unidad
  float v = atof(kp.t);
  bool bad = kp.t[0] && (v < kp.lo || v > kp.hi);
  char r[48];
  lv_snprintf(r, sizeof(r), kp.dec ? "%s %.2f - %.2f %s" : "%s %.0f - %.0f %s", bad ? "Fuera de rango:" : "Rango", kp.lo, kp.hi, kp.unitTxt);
  lv_label_set_text(kp.hint, r);
  lv_obj_set_style_text_color(kp.hint, bad ? lv_color_hex(0xFF8A70) : C_MUTED, 0);
}

static void kp_close() { lv_obj_delete_async(kp.scrim); kp.scrim = nullptr; }
static void kp_cancel_cb(lv_event_t*) { kp_close(); }

static void kp_cb(lv_event_t* e) {
  const char* k = (const char*)lv_event_get_user_data(e);
  size_t n = strlen(kp.t);
  char* dot = strchr(kp.t, '.');
  if (strcmp(k, LV_SYMBOL_BACKSPACE) == 0) { if (n) kp.t[n - 1] = 0; }
  else if (strcmp(k, LV_SYMBOL_OK) == 0) {
    float v = atof(kp.t);
    if (kp.t[0] && v >= kp.lo && v <= kp.hi) { NumCb f = kp.ok; kp_close(); f(v); }
    return;
  }
  else if (k[0] == '.') { if (kp.dec && !dot && n < sizeof(kp.t) - 2) { if (!n) kp.t[n++] = '0'; kp.t[n++] = '.'; kp.t[n] = 0; } }
  else if (n < sizeof(kp.t) - 1 && !(dot && strlen(dot + 1) >= kp.dec) && n < 6) {
    if (n == 1 && kp.t[0] == '0') n = 0; // sin ceros a la izquierda
    kp.t[n] = k[0]; kp.t[n + 1] = 0;
  }
  kpDraw();
}

void uiKeypad(const char* title, const char* unit, float lo, float hi, uint8_t decimals, NumCb ok) {
  kp.t[0] = 0; kp.lo = lo; kp.hi = hi; kp.dec = decimals; kp.ok = ok; kp.unitTxt = unit;
  kp.scrim = makeScrim(LV_OPA_70);
  lv_obj_t* d = uiBox(kp.scrim, 16, 16, 768, 448, C_CARD, 16);
  uiSetText(uiLabel(d, &lv_font_montserrat_24, C_TEXT), "%s", title);
  lv_obj_set_pos(lv_obj_get_child(d, 0), 16, 16);
  lv_obj_t* disp = uiBox(d, 16, 56, 280, 96, C_DARK, 12);
  kp.unit = uiLabel(disp, &lv_font_montserrat_24, C_MUTED, unit);
  lv_obj_align(kp.unit, LV_ALIGN_RIGHT_MID, -16, 6);
  kp.val = uiLabel(disp, &lv_font_montserrat_48, C_TEXT, "0");
  kp.hint = uiLabel(d, &lv_font_montserrat_18, C_MUTED);
  lv_obj_set_pos(kp.hint, 16, 166);
  lv_obj_set_width(kp.hint, 280);
  lv_label_set_long_mode(kp.hint, LV_LABEL_LONG_WRAP);
  lv_obj_t* c = uiBtn(d, C_BG);
  lv_obj_set_pos(c, 16, 368);
  lv_obj_set_size(c, 280, 64);
  uiLabel(c, &lv_font_montserrat_20, C_TEXT, LV_SYMBOL_CLOSE "  Cancelar");
  lv_obj_add_event_cb(c, kp_cancel_cb, LV_EVENT_CLICKED, nullptr);

  // Rejilla de 3x5 en x 312..752, y 16..432: números en gris, borrar en rojo y OK en verde
  const int KW = 141, KH = 76, G = 8;
  for (int i = 0; i < 13; i++) {
    const char* k = kpKeys[i];
    bool del = i == 11, okKey = i == 12;
    lv_obj_t* b = uiBtn(d, del ? C_RED : okKey ? C_GREEN : C_BG, okKey ? C_DARK : lv_color_white());
    lv_obj_set_style_radius(b, 12, 0);
    lv_obj_set_pos(b, 312 + (i % 3) * (KW + G), 16 + (i / 3) * (KH + G));
    lv_obj_set_size(b, okKey ? 3 * KW + 2 * G : KW, KH);
    uiLabel(b, &lv_font_montserrat_32, okKey ? C_DARK : del ? lv_color_white() : C_TEXT, k);
    lv_obj_add_event_cb(b, kp_cb, LV_EVENT_CLICKED, (void*)k);
  }
  kpDraw();
}

// ---------------------------------------------------------------------------
// Hoja inferior
// ---------------------------------------------------------------------------
static lv_obj_t* sheetScrim = nullptr;
static VoidCb sheetRefresh = nullptr;

static void sheet_scrim_cb(lv_event_t* e) {
  if (lv_event_get_target(e) == lv_event_get_current_target(e)) uiSheetClose();
}
static void sheet_close_cb(lv_event_t*) { uiSheetClose(); }

lv_obj_t* uiSheetOpen(const char* title, const char* sub, VoidCb onRefresh, lv_obj_t** extraSlot) {
  uiSheetClose();
  sheetRefresh = onRefresh;
  sheetScrim = makeScrim(LV_OPA_60);
  lv_obj_add_event_cb(sheetScrim, sheet_scrim_cb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* p = lv_obj_create(sheetScrim);
  lv_obj_remove_style_all(p);
  lv_obj_set_size(p, 800, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(p, C_CARD, 0);
  lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(p, 16, 0);
  lv_obj_set_style_pad_all(p, 12, 0);
  lv_obj_set_style_pad_bottom(p, 28, 0); // el radio inferior queda fuera de pantalla
  lv_obj_set_style_pad_gap(p, 10, 0);
  lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
  lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(p, LV_ALIGN_BOTTOM_MID, 0, 16);

  lv_obj_t* h = uiRow(p, 12);
  lv_obj_t* tc = lv_obj_create(h);
  lv_obj_remove_style_all(tc);
  lv_obj_set_size(tc, 10, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(tc, 1);
  lv_obj_set_flex_flow(tc, LV_FLEX_FLOW_COLUMN);
  lv_obj_remove_flag(tc, LV_OBJ_FLAG_CLICKABLE);
  uiLabel(tc, &lv_font_montserrat_24, C_TEXT, title);
  if (sub && *sub) uiLabel(tc, &lv_font_montserrat_16, C_MUTED, sub);
  if (extraSlot) { *extraSlot = uiRow(h, 8); lv_obj_set_width(*extraSlot, LV_SIZE_CONTENT); }
  lv_obj_t* x = uiBtn(h, C_CARD, C_MUTED);
  lv_obj_set_size(x, 64, 64);
  uiLabel(x, &lv_font_montserrat_24, C_MUTED, LV_SYMBOL_CLOSE);
  lv_obj_add_event_cb(x, sheet_close_cb, LV_EVENT_CLICKED, nullptr);
  return p;
}

void uiSheetClose() {
  if (!sheetScrim) return;
  lv_obj_delete_async(sheetScrim);
  sheetScrim = nullptr;
  sheetRefresh = nullptr;
}

void uiSheetRefresh() { if (sheetRefresh) sheetRefresh(); }
bool uiSheetIsOpen() { return sheetScrim != nullptr; }
