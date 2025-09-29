#include "UI/components/homeTab.h"
#include "Commons/globals.h"

lv_obj_t *diameterSpinboxMinBtn, *diameterSpinboxMaxBtn;

static void hook_division_lines(lv_event_t *e);
static void draw_event_cb(lv_event_t *e);

static void reset_measure_btn_cb(lv_event_t *e) {
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
    uint32_t currentTab = lv_tabview_get_tab_active(tabview);
    // Almacenar el tab actual en el dato de usuario del tabview
    lv_obj_set_user_data(tabview, (void*)(uintptr_t)currentTab);
    
    setConfirmationCallback([](uint32_t originalTab) {
      measurement.reset();
      lv_tabview_set_act(tabview, originalTab, LV_ANIM_OFF);
    }, currentTab, "Continuar con el reset de medidas?");
    
    lv_tabview_set_act(tabview, 3, LV_ANIM_OFF);
  }
}

static void lv_diameter_spinbox_increment_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    lv_spinbox_increment(diameterSpinbox);

    updateDiameter(lv_spinbox_get_value(diameterSpinbox) / 100.0f);
  }
}

static void lv_diameter_spinbox_decrement_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    lv_spinbox_decrement(diameterSpinbox);

    updateDiameter(lv_spinbox_get_value(diameterSpinbox) / 100.0f);
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
  lv_obj_add_event_cb(resetMeasureBtn, reset_measure_btn_cb, LV_EVENT_CLICKED, NULL);
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
  lv_obj_set_size(winderInfoParent, INFO_PARENT, 180);
  lv_obj_set_style_pad_all(winderInfoParent, 20, 0);
  lv_obj_clear_flag(winderInfoParent, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *winderInfoPullerSpeedLabelIcon = lv_label_create(winderInfoParent);
  lv_obj_set_style_text_font(winderInfoPullerSpeedLabelIcon, &icons, 0);
  lv_obj_set_style_text_color(winderInfoPullerSpeedLabelIcon, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(winderInfoPullerSpeedLabelIcon, SPEED_SYMBOL);
  lv_obj_align(winderInfoPullerSpeedLabelIcon, LV_ALIGN_TOP_LEFT, 0, 10);

  winderInfoPullerSpeedLabel = lv_label_create(winderInfoParent);
  lv_obj_set_style_text_font(winderInfoPullerSpeedLabel, &lv_font_montserrat_42, 0);
  lv_obj_set_style_text_color(winderInfoPullerSpeedLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(winderInfoPullerSpeedLabel, "0");
  lv_obj_align_to(winderInfoPullerSpeedLabel, winderInfoPullerSpeedLabelIcon, LV_ALIGN_LEFT_MID, 80, 0);

  winderInfoWeightLabelIcon = lv_label_create(winderInfoParent);
  lv_obj_set_style_text_font(winderInfoWeightLabelIcon, &icons, 0);
  lv_obj_set_style_text_color(winderInfoWeightLabelIcon, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(winderInfoWeightLabelIcon, WEIGHT_SYMBOL);
  lv_obj_align_to(winderInfoWeightLabelIcon, winderInfoPullerSpeedLabelIcon, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);

  winderInfoWeightLabel = lv_label_create(winderInfoParent);
  lv_obj_set_style_text_font(winderInfoWeightLabel, &lv_font_montserrat_42, 0);
  lv_obj_set_style_text_color(winderInfoWeightLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(winderInfoWeightLabel, "0 g");
  lv_obj_align_to(winderInfoWeightLabel, winderInfoPullerSpeedLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 25);

  winderInfoWeightLabelType = lv_label_create(winderInfoParent);
  lv_obj_set_style_text_font(winderInfoWeightLabelType, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(winderInfoWeightLabelType, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(winderInfoWeightLabelType, plasticTypes[selectedPlasticTypeIndex].name.c_str());
  lv_obj_align_to(winderInfoWeightLabelType, winderInfoWeightLabelIcon, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

  lv_obj_t *winderFilamentParent = lv_obj_create(parent);
  lv_obj_set_style_bg_color(winderFilamentParent, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(winderFilamentParent, 0, 0);
  lv_obj_align_to(winderFilamentParent, winderInfoParent, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);
  lv_obj_set_size(winderFilamentParent, lv_obj_get_width(tab1) - lv_obj_get_width(winderInfoParent) - 30, 180);

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
  lv_obj_align(diameterSpinbox, LV_ALIGN_TOP_MID, 0, 60);
  lv_spinbox_set_value(diameterSpinbox, diameter * 100);

  diameterSpinboxMinBtn = lv_btn_create(winderFilamentParent);
  lv_obj_set_size(diameterSpinboxMinBtn, lv_obj_get_height(diameterSpinbox), lv_obj_get_height(diameterSpinbox));
  lv_obj_align_to(diameterSpinboxMinBtn, diameterSpinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_set_style_bg_img_src(diameterSpinboxMinBtn, LV_SYMBOL_MINUS, 0);
  lv_obj_set_style_bg_color(diameterSpinboxMinBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(diameterSpinboxMinBtn, lv_diameter_spinbox_decrement_event_cb, LV_EVENT_CLICKED,  NULL);

  diameterSpinboxMaxBtn = lv_btn_create(winderFilamentParent);
  lv_obj_set_size(diameterSpinboxMaxBtn, lv_obj_get_height(diameterSpinbox), lv_obj_get_height(diameterSpinbox));
  lv_obj_align_to(diameterSpinboxMaxBtn, diameterSpinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
  lv_obj_set_style_bg_img_src(diameterSpinboxMaxBtn, LV_SYMBOL_PLUS, 0);
  lv_obj_set_style_bg_color(diameterSpinboxMaxBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(diameterSpinboxMaxBtn, lv_diameter_spinbox_increment_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *bottomPanel = lv_obj_create(parent);
  lv_obj_set_size(bottomPanel, lv_obj_get_width(lv_scr_act()) - SIDEBAR_WIDTH - 20, 135);
  lv_obj_align(bottomPanel, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  lv_obj_set_style_bg_color(bottomPanel, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(bottomPanel, 0, 0);
  lv_obj_clear_flag(bottomPanel, LV_OBJ_FLAG_SCROLLABLE);

  chartMaxLabel = lv_label_create(bottomPanel);
  lv_obj_set_style_text_color(chartMaxLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(chartMaxLabel, LV_ALIGN_TOP_LEFT, -5, -8);
  lv_obj_set_style_text_font(chartMaxLabel, &lv_font_montserrat_18, 0);
  lv_obj_set_style_bg_color(chartMaxLabel, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_bg_opa(chartMaxLabel, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_right(chartMaxLabel, 10, 0);
  
  chartMinLabel = lv_label_create(bottomPanel);
  lv_obj_set_style_text_color(chartMinLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(chartMinLabel, LV_ALIGN_BOTTOM_LEFT, -5, 8);
  lv_obj_set_style_text_font(chartMinLabel, &lv_font_montserrat_18, 0);
  lv_obj_set_style_bg_color(chartMinLabel, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_bg_opa(chartMinLabel, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_right(chartMinLabel, 10, 0);

  chart = lv_chart_create(bottomPanel);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_obj_set_size(chart, lv_obj_get_width(lv_scr_act()) - SIDEBAR_WIDTH - 40, 100);
  lv_obj_set_style_pad_all(chart, 0, 0);
  lv_obj_set_style_radius(chart, 0, 0);
  lv_obj_center(chart);
  lv_obj_set_style_bg_color(chart, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(chart, 0, 0);

  lv_chart_set_point_count(chart, 50);
  lv_chart_set_div_line_count(chart, 11, 10);

  updateChartLimits();

  lv_obj_add_event_cb(chart, draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
  lv_obj_add_flag(chart, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

  ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_PRIMARY_Y);

  lv_obj_move_foreground(chartMinLabel);
  lv_obj_move_foreground(chartMaxLabel);
}

static void hook_division_lines(lv_event_t *e) {
  lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
  lv_draw_dsc_base_t *base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);
  lv_draw_line_dsc_t *line_dsc = (lv_draw_line_dsc_t *)lv_draw_task_get_draw_dsc(draw_task);

  if (line_dsc->p1.x == line_dsc->p2.x) {
    line_dsc->width = 0;
    return;
  } else {
    if (base_dsc->id1 == 5) {
      line_dsc->width = 2;
      line_dsc->color = lv_palette_main(LV_PALETTE_ORANGE);
      line_dsc->opa = LV_OPA_40;
      line_dsc->dash_gap = 0;
      line_dsc->dash_width = 0;
    } else if (base_dsc->id1 == 0 || base_dsc->id1 == 10) {
      line_dsc->width = 2;
      line_dsc->color = lv_palette_main(LV_PALETTE_GREY);
      line_dsc->dash_gap = 0;
      line_dsc->dash_width = 0;
    } else {
      line_dsc->width = 0;
    }
  }
}

static void draw_event_cb(lv_event_t *e) {
  lv_draw_task_t * draw_task = lv_event_get_draw_task(e);
  lv_draw_dsc_base_t * base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);

  if(base_dsc->part == LV_PART_MAIN && lv_draw_task_get_type(draw_task) == LV_DRAW_TASK_TYPE_LINE) {
    hook_division_lines(e);
  }
}

