#include <Screen/components/general.h>

Arduino_H7_Video Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch TouchDetector;

lv_obj_t *tabview, *tab1, *tab2, *tab3, *tab4;
lv_obj_t *minMeasureLabel, *actMeasureLabel, *maxMeasureLabel;
lv_obj_t *winderInfoPullerSpeedLabel, *winderInfoTimeLabel, *winderInfoWeightLabel, *waterTempInfoLabel;
lv_obj_t *activeTabIndex, *confirmationMenuAction;
lv_obj_t *minSpeedSpinbox, *maxSpeedSpinbox, *waterTempSpinbox, *diameterSpinbox, *autostopSpinbox, *autostopSpinboxLabel;
lv_obj_t *polymerDropdown;
lv_obj_t *positionBar, *homePositionBtn, *startAlignerBtn;

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

  lv_label_set_text_fmt(actMeasureLabel, "%.2f", measurement.lastRead);
  lv_label_set_text_fmt(minMeasureLabel, "%.2f", measurement.minRead);
  lv_label_set_text_fmt(maxMeasureLabel, "%.2f", measurement.maxRead);

  aligner.currentPosition();

  lv_bar_set_value(positionBar, abs(0), LV_ANIM_ON);

  lv_label_set_text(winderInfoTimeLabel, getTime(millis()));

  lv_label_set_text_fmt(winderInfoWeightLabel, "%.2f", getExtrudedWeight());
}

void setupUITimer() {
  lv_timer_create(update_ui, 100, NULL);
}