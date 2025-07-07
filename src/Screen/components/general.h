#pragma once

#include <lvgl.h>
#include <Arduino_H7_Video.h>
#include <Arduino_GigaDisplayTouch.h>
#include <Commons/Commons.h>

extern Arduino_H7_Video Display;
extern Arduino_GigaDisplayTouch TouchDetector;

//tachometer-alt, arrows-alt, weight-hanging
//0xf3fd, 0xf0b2, 0xf5cd
LV_FONT_DECLARE(icons);
#define SPEED_SYMBOL "\xEF\x8F\xBD"
#define MOVE_SYMBOL "\xEF\x82\xB2"
#define WEIGHT_SYMBOL "\xEF\x97\x8D"

#define SIDEBAR_WIDTH 110

extern lv_obj_t *tabview, *tab1, *tab2, *tab3, *tab4;
extern lv_obj_t *minMeasureLabel, *actMeasureLabel, *maxMeasureLabel;
extern lv_obj_t *winderInfoPullerSpeedLabel, *winderInfoWeightLabel, *winderInfoWeightLabelIcon, *winderInfoWeightLabelType;
extern lv_obj_t *activeTabIndex, *confirmationMenuAction;
extern lv_obj_t *minSpeedSpinbox, *maxSpeedSpinbox, *diameterSpinbox;
extern lv_obj_t *positionBar, *homePositionBtn, *startSpoolCalibrationBtn, *positionBarMinLabel, *positionBarMaxLabel;

extern lv_obj_t *chart, *chartMaxLabel, *chartMinLabel;
extern lv_chart_series_t *ser;

void setActiveTab(uint8_t index);

String getConfirmationMenuAction();

void setConfirmationMenuAction(String action);

void setupUITimer();

void addChartValue(float value);

void updateChartLimits();

void updateSpoolProgressLimits(int32_t min, int32_t max);