#include <UI/components/settingsTab.h>
#include "Commons/globals.h"

static void lv_min_spinbox_increment_event_cb(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  if(code == LV_EVENT_SHORT_CLICKED || code  == LV_EVENT_LONG_PRESSED_REPEAT) {
    lv_spinbox_increment(minSpeedSpinbox);

    puller.updateMinOutput(lv_spinbox_get_value(minSpeedSpinbox));
  }
}

static void lv_min_spinbox_decrement_event_cb(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  if(code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    lv_spinbox_decrement(minSpeedSpinbox);

    puller.updateMinOutput(lv_spinbox_get_value(minSpeedSpinbox));
  }
}

static void lv_max_spinbox_decrement_event_cb(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  if(code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    lv_spinbox_decrement(maxSpeedSpinbox);

    puller.updateMaxOutput(lv_spinbox_get_value(maxSpeedSpinbox));
  }
}

static void lv_max_spinbox_increment_event_cb(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  if(code == LV_EVENT_SHORT_CLICKED || code  == LV_EVENT_LONG_PRESSED_REPEAT) {
    lv_spinbox_increment(maxSpeedSpinbox);

    puller.updateMaxOutput(lv_spinbox_get_value(maxSpeedSpinbox));
  }
}

static void plastic_type_changed_event_cb(lv_event_t *event) {
  lv_obj_t *dropdown = (lv_obj_t *)lv_event_get_target(event);
  uint8_t selected = lv_dropdown_get_selected(dropdown);

  if (selected < plasticTypes.size()) {
    updatePlasticType(selected);

    lv_label_set_text_fmt(winderInfoWeightLabelType, "%s", plasticTypes[selected].name.c_str());
    lv_obj_align_to(winderInfoWeightLabelType, winderInfoWeightLabelIcon, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
  }
}

static void restart_system_cb(lv_event_t *event) {
  NVIC_SystemReset();
}

static void reinit_cb(lv_event_t *event) {
  aligner.reinit();
}

void build_settingsTab(lv_obj_t *parent) {
  lv_obj_t *settingsParent = lv_obj_create(parent);
  lv_obj_align(settingsParent, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_set_size(settingsParent, lv_obj_get_width(parent) - 20, lv_obj_get_height(parent) - 20);
  lv_obj_set_style_bg_color(settingsParent, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(settingsParent, 0, 0);
  lv_obj_remove_flag(settingsParent, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *minSpeedSpinboxLabel = lv_label_create(settingsParent);
  lv_obj_set_style_text_color(minSpeedSpinboxLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(minSpeedSpinboxLabel, LV_ALIGN_TOP_LEFT, 0, 20);
  lv_label_set_text(minSpeedSpinboxLabel, "Velocidad Min");

  minSpeedSpinbox = lv_spinbox_create(settingsParent);
  lv_spinbox_set_range(minSpeedSpinbox, 0, 13000);
  lv_spinbox_set_digit_format(minSpeedSpinbox, 5, 0);
  lv_spinbox_set_step(minSpeedSpinbox, 1000);
  lv_spinbox_step_prev(minSpeedSpinbox);
  lv_obj_set_width(minSpeedSpinbox, 150);
  lv_obj_align_to(minSpeedSpinbox, minSpeedSpinboxLabel, LV_ALIGN_OUT_RIGHT_MID, lv_obj_get_height(minSpeedSpinbox) + 135, 0);
  lv_spinbox_set_value(minSpeedSpinbox, puller.minOutput);

  lv_obj_t *minSpeedSpinboxPlusBtn = lv_btn_create(settingsParent);
  lv_obj_set_size(minSpeedSpinboxPlusBtn, lv_obj_get_height(minSpeedSpinbox), lv_obj_get_height(minSpeedSpinbox));
  lv_obj_align_to(minSpeedSpinboxPlusBtn, minSpeedSpinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
  lv_obj_set_style_bg_img_src(minSpeedSpinboxPlusBtn, LV_SYMBOL_PLUS, 0);
  lv_obj_set_style_bg_color(minSpeedSpinboxPlusBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(minSpeedSpinboxPlusBtn, lv_min_spinbox_increment_event_cb, LV_EVENT_ALL,  NULL);

  lv_obj_t *minSpeedSpinboxMinusBtn = lv_btn_create(settingsParent);
  lv_obj_set_size(minSpeedSpinboxMinusBtn, lv_obj_get_height(minSpeedSpinbox), lv_obj_get_height(minSpeedSpinbox));
  lv_obj_align_to(minSpeedSpinboxMinusBtn, minSpeedSpinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_set_style_bg_img_src(minSpeedSpinboxMinusBtn, LV_SYMBOL_MINUS, 0);
  lv_obj_set_style_bg_color(minSpeedSpinboxMinusBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(minSpeedSpinboxMinusBtn, lv_min_spinbox_decrement_event_cb, LV_EVENT_ALL, NULL);

  


  lv_obj_t *maxSpeedSpinboxLabel = lv_label_create(settingsParent);
  lv_obj_set_style_text_color(maxSpeedSpinboxLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(maxSpeedSpinboxLabel, "Velocidad Max");
  lv_obj_align(maxSpeedSpinboxLabel, LV_ALIGN_TOP_LEFT, 0, 100);

  maxSpeedSpinbox = lv_spinbox_create(settingsParent);
  lv_spinbox_set_range(maxSpeedSpinbox, 0, 13000);
  lv_spinbox_set_digit_format(maxSpeedSpinbox, 5, 0);
  lv_spinbox_step_prev(maxSpeedSpinbox);
  lv_spinbox_set_step(maxSpeedSpinbox, 1000);
  lv_obj_set_width(maxSpeedSpinbox, 150);
  lv_obj_align_to(maxSpeedSpinbox, minSpeedSpinbox, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
  lv_spinbox_set_value(maxSpeedSpinbox, puller.maxOutput);

  lv_obj_t *maxSpeedSpinboxPlusBtn = lv_btn_create(settingsParent);
  lv_obj_set_size(maxSpeedSpinboxPlusBtn, lv_obj_get_height(maxSpeedSpinbox), lv_obj_get_height(maxSpeedSpinbox));
  lv_obj_align_to(maxSpeedSpinboxPlusBtn, maxSpeedSpinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
  lv_obj_set_style_bg_img_src(maxSpeedSpinboxPlusBtn, LV_SYMBOL_PLUS, 0);
  lv_obj_set_style_bg_color(maxSpeedSpinboxPlusBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(maxSpeedSpinboxPlusBtn, lv_max_spinbox_increment_event_cb, LV_EVENT_ALL,  NULL);

  lv_obj_t *maxSpeedSpinboxMinusBtn = lv_btn_create(settingsParent);
  lv_obj_set_size(maxSpeedSpinboxMinusBtn, lv_obj_get_height(maxSpeedSpinbox), lv_obj_get_height(maxSpeedSpinbox));
  lv_obj_align_to(maxSpeedSpinboxMinusBtn, maxSpeedSpinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_set_style_bg_img_src(maxSpeedSpinboxMinusBtn, LV_SYMBOL_MINUS, 0);
  lv_obj_set_style_bg_color(maxSpeedSpinboxMinusBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(maxSpeedSpinboxMinusBtn, lv_max_spinbox_decrement_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *plasticTypeLabel = lv_label_create(settingsParent);
  lv_obj_set_style_text_color(plasticTypeLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(plasticTypeLabel, "Tipo de plastico");
  lv_obj_align(plasticTypeLabel, LV_ALIGN_TOP_LEFT, 0, 180);

  lv_obj_t *plasticTypeDropdown = lv_dropdown_create(settingsParent);
  std::string options;
  for (size_t i = 0; i < plasticTypes.size(); ++i) {
    options += plasticTypes[i].name.c_str();
    if (i != plasticTypes.size() - 1) options += "\n";
  }
  lv_dropdown_set_options(plasticTypeDropdown, options.c_str());
  lv_obj_set_width(plasticTypeDropdown, 280);
  lv_obj_align_to(plasticTypeDropdown, maxSpeedSpinboxMinusBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
  lv_dropdown_set_selected(plasticTypeDropdown, selectedPlasticTypeIndex);
  lv_obj_add_event_cb(plasticTypeDropdown, plastic_type_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t *restartSystemBtn = lv_btn_create(settingsParent);
  lv_obj_align(restartSystemBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(restartSystemBtn, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_add_event_cb(restartSystemBtn, restart_system_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *restartSystemBtnLabel = lv_label_create(restartSystemBtn);
  lv_obj_set_style_text_color(restartSystemBtnLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(restartSystemBtnLabel, LV_SYMBOL_REFRESH " Reiniciar");

  lv_obj_t *reinitBtn = lv_btn_create(settingsParent);
  lv_obj_align(reinitBtn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_bg_color(reinitBtn, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_add_event_cb(reinitBtn, reinit_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *reinitBtnLabel = lv_label_create(reinitBtn);
  lv_obj_set_style_text_color(reinitBtnLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(reinitBtnLabel, "Reiniciar Aligner");
}