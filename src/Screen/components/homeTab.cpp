#include "Screen/components/homeTab.h"

lv_obj_t *diameterSpinboxMinBtn, *diameterSpinboxMaxBtn, *autostopSpinboxMinBtn, *autostopSpinboxMaxBtn;

static void reset_measure_btn_cb(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);


  if (code == LV_EVENT_PRESSED) {
    lv_obj_set_style_text_color(lv_obj_get_child(btn, 0), lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  } else if (code == LV_EVENT_RELEASED) {
    lv_obj_set_style_text_color(lv_obj_get_child(btn, 0), lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
  } else if(code == LV_EVENT_CLICKED) {
    setActiveTab(0);
    setConfirmationMenuAction("resetMeasure");
    lv_tabview_set_act(tabview, 3, LV_ANIM_OFF);
  }
}

static void lv_diameter_spinbox_increment_event_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED) {
    lv_spinbox_increment(diameterSpinbox);

    updateDiameter(lv_spinbox_get_value(diameterSpinbox) / 100.0f);
  }
}

static void lv_diameter_spinbox_decrement_event_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED) {
    lv_spinbox_decrement(diameterSpinbox);

    updateDiameter(lv_spinbox_get_value(diameterSpinbox) / 100.0f);
  }
}

static void lv_autostop_spinbox_increment_event_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED) {
    lv_spinbox_increment(autostopSpinbox);

    updateAutostop(lv_spinbox_get_value(autostopSpinbox) / 100.0f);
  }
}

static void lv_autostop_spinbox_decrement_event_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED) {
    lv_spinbox_decrement(autostopSpinbox);

    updateAutostop(lv_spinbox_get_value(autostopSpinbox) / 100.0f);
  }
}

void build_homeTab(lv_obj_t *parent) {
  static lv_style_t measureLabelStyle;
  lv_style_init(&measureLabelStyle);
  lv_style_set_text_color(&measureLabelStyle, lv_color_hex(0xFFFFFF));
  lv_style_set_text_font(&measureLabelStyle, &lv_font_montserrat_48);

  lv_obj_t *measuringParent = lv_obj_create(parent);
  lv_obj_align(measuringParent, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_set_size(measuringParent, lv_obj_get_width(parent) - SIDEBAR_WIDTH - 25, 125);
  lv_obj_set_style_bg_color(measuringParent, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(measuringParent, 0, 0);

  minMeasureLabel = lv_label_create(measuringParent);
  lv_obj_align(minMeasureLabel, LV_ALIGN_LEFT_MID, 30, 0);
  lv_obj_add_style(minMeasureLabel, &measureLabelStyle, 0);
  lv_label_set_text(minMeasureLabel, "0.00");

  actMeasureLabel = lv_label_create(measuringParent);
  lv_obj_align(actMeasureLabel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(actMeasureLabel, &measureLabelStyle, 0);
  lv_label_set_text(actMeasureLabel, "0.00");
  
  maxMeasureLabel = lv_label_create(measuringParent);
  lv_obj_align(maxMeasureLabel, LV_ALIGN_RIGHT_MID, -30, 0);
  lv_obj_add_style(maxMeasureLabel, &measureLabelStyle, 0);
  lv_label_set_text(maxMeasureLabel, "0.00");

  lv_obj_t *measuringActionButtonsParent = lv_obj_create(parent);
  lv_obj_align_to(measuringActionButtonsParent, measuringParent, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);
  lv_obj_set_size(measuringActionButtonsParent, 125, 125);
  lv_obj_set_style_bg_color(measuringActionButtonsParent, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(measuringActionButtonsParent, 0, 0);
  lv_obj_set_style_pad_all(measuringActionButtonsParent, 0, 0);

  lv_obj_t *resetMeasureBtn = lv_btn_create(measuringActionButtonsParent);
  lv_obj_remove_style_all(resetMeasureBtn);
  lv_obj_set_size(resetMeasureBtn, 125, 125);
  lv_obj_align(resetMeasureBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(resetMeasureBtn, reset_measure_btn_cb, LV_EVENT_ALL, NULL);
  lv_obj_set_style_radius(resetMeasureBtn, 10, 0);

  lv_obj_t *resetMeasureBtnLabel = lv_label_create(resetMeasureBtn);
  lv_label_set_text(resetMeasureBtnLabel, LV_SYMBOL_REFRESH);
  lv_obj_set_style_text_font(resetMeasureBtnLabel, &lv_font_montserrat_48, 0);
  lv_obj_align(resetMeasureBtnLabel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(resetMeasureBtnLabel, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *winderInfoParent = lv_obj_create(parent);
  lv_obj_set_style_bg_color(winderInfoParent, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(winderInfoParent, 0, 0);
  lv_obj_align_to(winderInfoParent, measuringParent, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_set_size(winderInfoParent, INFO_PARENT, LV_VER_RES - lv_obj_get_height(measuringParent) - 30);

  lv_obj_t *winderInfoPullerSpeedLabelIcon = lv_label_create(winderInfoParent);
  lv_obj_set_style_text_font(winderInfoPullerSpeedLabelIcon, &icons, 0);
  lv_obj_set_style_text_color(winderInfoPullerSpeedLabelIcon, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(winderInfoPullerSpeedLabelIcon, SPEED_SYMBOL);

  winderInfoPullerSpeedLabel = lv_label_create(winderInfoParent);
  lv_obj_set_style_text_font(winderInfoPullerSpeedLabel, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(winderInfoPullerSpeedLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(winderInfoPullerSpeedLabel, "0");
  lv_obj_align_to(winderInfoPullerSpeedLabel, winderInfoPullerSpeedLabelIcon, LV_ALIGN_LEFT_MID, 70, 0);

  lv_obj_t *winderInfoWeightLabelIcon = lv_label_create(winderInfoParent);
  lv_obj_set_style_text_font(winderInfoWeightLabelIcon, &icons, 0);
  lv_obj_set_style_text_color(winderInfoWeightLabelIcon, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(winderInfoWeightLabelIcon, WEIGHT_SYMBOL);
  lv_obj_align_to(winderInfoWeightLabelIcon, winderInfoPullerSpeedLabelIcon, LV_ALIGN_BOTTOM_LEFT, 0, lv_obj_get_height(winderInfoPullerSpeedLabelIcon) + 25);

  winderInfoWeightLabel = lv_label_create(winderInfoParent);
  lv_obj_set_style_text_font(winderInfoWeightLabel, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(winderInfoWeightLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(winderInfoWeightLabel, "0");
  lv_obj_align_to(winderInfoWeightLabel, winderInfoWeightLabelIcon, LV_ALIGN_LEFT_MID, 70, 0);

  lv_obj_t *winderInfoTimeLabelIcon = lv_label_create(winderInfoParent);
  lv_obj_set_style_text_font(winderInfoTimeLabelIcon, &icons, 0);
  lv_obj_set_style_text_color(winderInfoTimeLabelIcon, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(winderInfoTimeLabelIcon, CLOCK_SYMBOL);
  lv_obj_align_to(winderInfoTimeLabelIcon, winderInfoWeightLabelIcon, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 25);

  winderInfoTimeLabel = lv_label_create(winderInfoParent);
  lv_obj_set_style_text_font(winderInfoTimeLabel, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(winderInfoTimeLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(winderInfoTimeLabel, "0");
  lv_obj_align_to(winderInfoTimeLabel, winderInfoTimeLabelIcon, LV_ALIGN_LEFT_MID, 70, 0);

  lv_obj_t *winderFilamentParent = lv_obj_create(parent);
  lv_obj_set_style_bg_color(winderFilamentParent, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(winderFilamentParent, 0, 0);
  lv_obj_align_to(winderFilamentParent, winderInfoParent, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);
  lv_obj_set_size(winderFilamentParent, lv_obj_get_width(tab1) - lv_obj_get_width(winderInfoParent) - 30, LV_VER_RES - lv_obj_get_height(measuringParent) - 30);

  lv_obj_t *diameterSpinboxLabel = lv_label_create(winderFilamentParent);
  lv_label_set_text(diameterSpinboxLabel, "Diametro");
  lv_obj_set_style_text_color(diameterSpinboxLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(diameterSpinboxLabel, LV_ALIGN_TOP_LEFT, 0, 0);

  diameterSpinbox = lv_spinbox_create(winderFilamentParent);
  lv_spinbox_set_step(diameterSpinbox, 0);
  lv_spinbox_set_range(diameterSpinbox, 0, 300);
  lv_spinbox_set_digit_format(diameterSpinbox, 3, 1);
  lv_spinbox_step_prev(diameterSpinbox);
  lv_obj_set_width(diameterSpinbox, 100);
  lv_obj_align(diameterSpinbox, LV_ALIGN_TOP_MID, 0, 50);
  lv_spinbox_set_value(diameterSpinbox, diameter * 100);

  diameterSpinboxMinBtn = lv_btn_create(winderFilamentParent);
  lv_obj_set_size(diameterSpinboxMinBtn, lv_obj_get_height(diameterSpinbox), lv_obj_get_height(diameterSpinbox));
  lv_obj_align_to(diameterSpinboxMinBtn, diameterSpinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_set_style_bg_img_src(diameterSpinboxMinBtn, LV_SYMBOL_MINUS, 0);
  lv_obj_set_style_bg_color(diameterSpinboxMinBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(diameterSpinboxMinBtn, lv_diameter_spinbox_decrement_event_cb, LV_EVENT_ALL,  NULL);

  diameterSpinboxMaxBtn = lv_btn_create(winderFilamentParent);
  lv_obj_set_size(diameterSpinboxMaxBtn, lv_obj_get_height(diameterSpinbox), lv_obj_get_height(diameterSpinbox));
  lv_obj_align_to(diameterSpinboxMaxBtn, diameterSpinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
  lv_obj_set_style_bg_img_src(diameterSpinboxMaxBtn, LV_SYMBOL_PLUS, 0);
  lv_obj_set_style_bg_color(diameterSpinboxMaxBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(diameterSpinboxMaxBtn, lv_diameter_spinbox_increment_event_cb, LV_EVENT_ALL, NULL);

  autostopSpinboxLabel = lv_label_create(winderFilamentParent);
  lv_label_set_text(autostopSpinboxLabel, "Auto Stop");
  lv_obj_set_style_text_color(autostopSpinboxLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(autostopSpinboxLabel, LV_ALIGN_TOP_LEFT, 0, 150);

  autostopSpinbox = lv_spinbox_create(winderFilamentParent);
  lv_spinbox_set_step(autostopSpinbox, 0);
  lv_spinbox_set_range(autostopSpinbox, 0, 125);
  lv_spinbox_set_digit_format(autostopSpinbox, 3, 1);
  lv_spinbox_step_prev(autostopSpinbox);
  lv_obj_set_width(autostopSpinbox, 100);
  lv_obj_align(autostopSpinbox, LV_ALIGN_TOP_MID, 0, 200);
  lv_spinbox_set_value(autostopSpinbox, autostop * 100);

  autostopSpinboxMinBtn = lv_btn_create(winderFilamentParent);
  lv_obj_set_size(autostopSpinboxMinBtn, lv_obj_get_height(autostopSpinbox), lv_obj_get_height(autostopSpinbox));
  lv_obj_align_to(autostopSpinboxMinBtn, autostopSpinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_set_style_bg_img_src(autostopSpinboxMinBtn, LV_SYMBOL_MINUS, 0);
  lv_obj_set_style_bg_color(autostopSpinboxMinBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(autostopSpinboxMinBtn, lv_autostop_spinbox_decrement_event_cb, LV_EVENT_ALL,  NULL);

  autostopSpinboxMaxBtn = lv_btn_create(winderFilamentParent);
  lv_obj_set_size(autostopSpinboxMaxBtn, lv_obj_get_height(autostopSpinbox), lv_obj_get_height(autostopSpinbox));
  lv_obj_align_to(autostopSpinboxMaxBtn, autostopSpinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
  lv_obj_set_style_bg_img_src(autostopSpinboxMaxBtn, LV_SYMBOL_PLUS, 0);
  lv_obj_set_style_bg_color(autostopSpinboxMaxBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(autostopSpinboxMaxBtn, lv_autostop_spinbox_increment_event_cb, LV_EVENT_ALL, NULL);
}