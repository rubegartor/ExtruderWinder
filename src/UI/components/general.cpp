#include <UI/components/general.h>
#include <UI/components/settingsTab.h>
#include "Commons/globals.h"

Arduino_H7_Video Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch TouchDetector;

lv_obj_t *tabview, *tab1, *tab2, *tab3, *tab4;
lv_obj_t *popupInfoLabel;
lv_obj_t *minMeasureLabel, *actMeasureLabel, *maxMeasureLabel;
lv_obj_t *winderInfoPullerSpeedLabel, *winderInfoWeightLabel, *winderInfoWeightLabelIcon, *winderInfoWeightLabelType;
lv_obj_t *minSpeedSpinbox, *maxSpeedSpinbox, *diameterSpinbox, *alertWeightSpinbox;
lv_obj_t *positionBar, *homePositionBtn, *startSpoolCalibrationBtn, *positionBarMinLabel, *positionBarMaxLabel;
lv_obj_t *moveLeftBtn, *moveRightBtn;
lv_obj_t *chart, *chartMaxLabel, *chartMinLabel;
lv_chart_series_t *ser;

static ConfirmationCallback currentConfirmationCallback = nullptr;
static uint32_t originalTabIndex = 0;

// Límites del positionBar pendientes de aplicar. updateSpoolProgressLimits()
// puede invocarse desde el motorTask, pero LVGL NO es thread-safe: por eso solo
// guardamos los valores y el cambio se aplica en update_ui() (hilo principal).
static volatile bool    spoolLimitsDirty = false;
static volatile int32_t pendingSpoolMin  = 0;
static volatile int32_t pendingSpoolMax  = 0;

void setConfirmationCallback(ConfirmationCallback callback, uint32_t originalTab, const char* message) {
  currentConfirmationCallback = callback;
  originalTabIndex = originalTab;
  lv_label_set_text(popupInfoLabel, message);
}

void executeConfirmationCallback() {
  if (currentConfirmationCallback != nullptr) {
    currentConfirmationCallback(originalTabIndex);
    currentConfirmationCallback = nullptr;
    originalTabIndex = 0;
  }
}

void clearConfirmationCallback() {
  currentConfirmationCallback = nullptr;
  originalTabIndex = 0;
}

void update_ui(lv_timer_t *timer) {
  lv_label_set_text_fmt(winderInfoPullerSpeedLabel, "%d", puller.speed);
  lv_label_set_text_fmt(winderInfoWeightLabel, "%.1f g", getExtrudedFilamentWeight());

  lv_label_set_text_fmt(actMeasureLabel, "%.2f", measurementLastRead);
  lv_label_set_text_fmt(minMeasureLabel, "%.2f", measurementMinRead);
  lv_label_set_text_fmt(maxMeasureLabel, "%.2f", measurementMaxRead);

  if (!aligner.isHoming() && positionBar) {
    lv_bar_set_value(positionBar, aligner.currentPosition(), LV_ANIM_ON);
  }

  if (startExtensionLabel) {
    int32_t startSteps = aligner.getStartExtensionSteps();
    lv_label_set_text_fmt(startExtensionLabel, startSteps >= 0 ? "Inicio: +%d" : "Inicio: %d", (int)startSteps);
    if (moveLeftBtn) {
      lv_obj_align_to(startExtensionLabel, moveLeftBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    }
  }

  if (endExtensionLabel) {
    int32_t endSteps = aligner.getEndExtensionSteps();
    lv_label_set_text_fmt(endExtensionLabel, endSteps >= 0 ? "Final: +%d" : "Final: %d", (int)endSteps);
    if (moveRightBtn) {
      lv_obj_align_to(endExtensionLabel, moveRightBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    }
  }

  if (g_newMeasurementReady) {
    g_newMeasurementReady = false;
    addChartValue(static_cast<int32_t>(measurementLastRead * 100));
  }

  // Aplicar límites del positionBar solicitados desde otros hilos (motorTask).
  if (spoolLimitsDirty) {
    spoolLimitsDirty = false;
    if (positionBar) {
      lv_bar_set_range(positionBar, pendingSpoolMin, pendingSpoolMax);
      lv_bar_set_value(positionBar, pendingSpoolMin, LV_ANIM_OFF);
    }
  }
}

void setupUITimer() {
  lv_timer_create(update_ui, 100, NULL);
}

void addChartValue(int32_t value) {
  if (chart && ser) {
    lv_chart_set_next_value(chart, ser, value);
  }
}

void updateChartLimits() {
  if (chart && chartMaxLabel && chartMinLabel) {
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, static_cast<int32_t>((diameter - 0.10) * 100), static_cast<int32_t>((diameter + 0.10) * 100));

    lv_label_set_text_fmt(chartMaxLabel, "%.2f", (diameter + 0.10));
    lv_label_set_text_fmt(chartMinLabel, "%.2f", (diameter - 0.10));
  }
}

void updateSpoolProgressLimits(int32_t min, int32_t max) {
  // Thread-safe: puede llamarse desde el motorTask. No toca LVGL aquí;
  // update_ui() (hilo principal) aplica el cambio cuando detecta el flag.
  pendingSpoolMin  = min;
  pendingSpoolMax  = max;
  spoolLimitsDirty = true;
}