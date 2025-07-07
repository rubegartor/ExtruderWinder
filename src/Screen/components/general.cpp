#include <Screen/components/general.h>

Arduino_H7_Video Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch TouchDetector;

lv_obj_t *tabview, *tab1, *tab2, *tab3, *tab4;
lv_obj_t *minMeasureLabel, *actMeasureLabel, *maxMeasureLabel;
lv_obj_t *winderInfoPullerSpeedLabel, *winderInfoWeightLabel, *winderInfoWeightLabelIcon, *winderInfoWeightLabelType;
lv_obj_t *activeTabIndex, *confirmationMenuAction;
lv_obj_t *minSpeedSpinbox, *maxSpeedSpinbox, *diameterSpinbox;
lv_obj_t *positionBar, *homePositionBtn, *startSpoolCalibrationBtn, *positionBarMinLabel, *positionBarMaxLabel;

lv_obj_t *chart, *chartMaxLabel, *chartMinLabel;
lv_chart_series_t *ser;

void setActiveTab(uint8_t index) {
  lv_label_set_text(activeTabIndex, String(index).c_str());
}

String getConfirmationMenuAction() {
  return String(lv_label_get_text(confirmationMenuAction));
}

void setConfirmationMenuAction(String action) {
  lv_label_set_text(confirmationMenuAction, action.c_str());
}

void update_ui(lv_timer_t *timer) {
  lv_label_set_text_fmt(winderInfoPullerSpeedLabel, "%d", puller.speed);
  lv_label_set_text_fmt(winderInfoWeightLabel, "%.1f g", getExtrudedFilamentWeight());

  lv_label_set_text_fmt(actMeasureLabel, "%.2f", measurement.lastRead);
  lv_label_set_text_fmt(minMeasureLabel, "%.2f", measurement.minRead);
  lv_label_set_text_fmt(maxMeasureLabel, "%.2f", measurement.maxRead);

  if (!aligner.isHoming()) {
    lv_bar_set_value(positionBar, aligner.currentPosition(), LV_ANIM_ON);
    lv_label_set_text_fmt(positionBarMinLabel, "0%% (%d)", aligner_left_pos);
    lv_label_set_text_fmt(positionBarMaxLabel, "100%% (%d)", aligner_right_pos);
  }
}

void setupUITimer() {
  lv_timer_create(update_ui, 100, NULL);
}

void addChartValue(float value) {
  if (chart && ser) {
    lv_chart_set_next_value(chart, ser, value);
  }
}

void updateChartLimits() {
  if (chart && chartMaxLabel && chartMinLabel) {
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, static_cast<int32_t>((diameter - 0.2) * 100), static_cast<int32_t>((diameter + 0.2) * 100));

    lv_label_set_text_fmt(chartMaxLabel, "%.2f", (diameter + 0.2));
    lv_label_set_text_fmt(chartMinLabel, "%.2f", (diameter - 0.2));
  }
}

void updateSpoolProgressLimits(int32_t min, int32_t max) {
  if (positionBar) {
    lv_bar_set_range(positionBar, min, max);
    lv_bar_set_value(positionBar, min, LV_ANIM_OFF);
  }
}