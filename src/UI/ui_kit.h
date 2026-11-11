#pragma once

#include <lvgl.h>

// Paleta de la UI (la misma que la del extrusor)
#define C_BG     lv_color_hex(0x2E3741)
#define C_DARK   lv_color_hex(0x252D36)
#define C_CARD   lv_color_hex(0x394450)
#define C_TEXT   lv_color_hex(0xE3E6E8)
#define C_SOFT   lv_color_hex(0xBFC3C6)
#define C_MUTED  lv_color_hex(0x8E99A4)
#define C_GRID   lv_color_hex(0x46525F)
#define C_ORANGE lv_color_hex(0xD96D30)
#define C_GREEN  lv_color_hex(0x82D930)
#define C_RED    lv_color_hex(0xD94E30)
#define C_YELLOW lv_color_hex(0xF39C12)
#define C_BLUE   lv_color_hex(0x2F6FA3)
#define C_FIL    lv_color_hex(0xFF7A00)

typedef void (*NumCb)(float value);
typedef void (*VoidCb)();

// Primitivas
lv_obj_t* uiBox(lv_obj_t* parent, int x, int y, int w, int h, lv_color_t bg, int radius = 0);
lv_obj_t* uiCircle(lv_obj_t* parent, float cx, float cy, float r, lv_color_t bg);
lv_obj_t* uiLabel(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, const char* text = "");
lv_obj_t* uiBtn(lv_obj_t* parent, lv_color_t bg, lv_color_t fg = C_TEXT, bool column = false);
lv_obj_t* uiRow(lv_obj_t* parent, int gap = 8);
void      uiSetText(lv_obj_t* label, const char* fmt, ...);
void      uiEnable(lv_obj_t* obj, bool enabled);

// Botón ± que repite al mantener y confirma (commit) al soltar
struct UiStepper { float* value; float delta, lo, hi; NumCb commit; };
void uiStepper(lv_obj_t* btn, UiStepper* cfg);

// Diálogos sobre lv_layer_top()
void uiToast(const char* msg, bool warn = false);
void uiConfirm(const char* msg, const char* sub, const char* yes, VoidCb ok);
void uiKeypad(const char* title, const char* unit, float lo, float hi, uint8_t decimals, NumCb ok);

// Hoja inferior: devuelve el panel (columna flex) donde añadir filas
lv_obj_t* uiSheetOpen(const char* title, const char* sub, VoidCb onRefresh, lv_obj_t** extraSlot = nullptr);
void      uiSheetClose();
void      uiSheetRefresh();
bool      uiSheetIsOpen();
