#include <UI/components/general.h>
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
lv_obj_t *wifiIPLabel;

lv_obj_t *chart, *chartMaxLabel, *chartMinLabel;
lv_chart_series_t *ser;

static ConfirmationCallback currentConfirmationCallback = nullptr;
static uint32_t originalTabIndex = 0;

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

  if (!aligner.isHoming()) {
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

  if (wifiIPLabel) {
    if (rpcManager.hasNewWebServerInfo()) {
      const WebServerMessage& config = rpcManager.getWebServerInfo();
      if (strlen(config.wifiIP) > 0) {
        lv_label_set_text_fmt(wifiIPLabel, "WiFi: %s", config.wifiIP);
      } else {
        lv_label_set_text(wifiIPLabel, "WiFi: Desconectado");
      }
      rpcManager.markWebServerInfoProcessed();
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
  if (positionBar) {
    lv_bar_set_range(positionBar, min, max);
    lv_bar_set_value(positionBar, min, LV_ANIM_OFF);
  }
}