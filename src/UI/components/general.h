#pragma once

#include <lvgl.h>
#include <Arduino_H7_Video.h>
#include <Arduino_GigaDisplayTouch.h>

//tachometer-alt, arrows-alt, weight-hanging, life-ring
//0xf3fd, 0xf0b2, 0xf5cd, 0xf1cd
LV_FONT_DECLARE(icons);
#define SPEED_SYMBOL "\xEF\x8F\xBD"
#define MOVE_SYMBOL "\xEF\x82\xB2"
#define WEIGHT_SYMBOL "\xEF\x97\x8D"
#define SPOOL_SYMBOL "\xEF\x87\x8D"

#define SIDEBAR_WIDTH 110


extern Arduino_H7_Video Display;
extern Arduino_GigaDisplayTouch TouchDetector;

// Callback que recibe el tab de origen para poder volver después
typedef void (*ConfirmationCallback)(uint32_t originalTab);

extern lv_obj_t *tabview, *tab1, *tab2, *tab3, *tab4;
extern lv_obj_t *popupInfoLabel;
extern lv_obj_t *minMeasureLabel, *actMeasureLabel, *maxMeasureLabel;
extern lv_obj_t *winderInfoPullerSpeedLabel, *winderInfoWeightLabel, *winderInfoWeightLabelIcon, *winderInfoWeightLabelType;
extern lv_obj_t *minSpeedSpinbox, *maxSpeedSpinbox, *diameterSpinbox;
extern lv_obj_t *positionBar, *homePositionBtn, *startSpoolCalibrationBtn, *positionBarMinLabel, *positionBarMaxLabel;
extern lv_obj_t *startExtensionLabel, *endExtensionLabel;
extern lv_obj_t *moveLeftBtn, *moveRightBtn; // Referencias para realineación de etiquetas

extern lv_obj_t *chart, *chartMaxLabel, *chartMinLabel;
extern lv_chart_series_t *ser;

void setConfirmationCallback(ConfirmationCallback callback, uint32_t originalTab, const char* message);
void executeConfirmationCallback();
void clearConfirmationCallback();

void setupUITimer();

void addChartValue(int32_t value);

void updateChartLimits();

void updateSpoolProgressLimits(int32_t min, int32_t max);