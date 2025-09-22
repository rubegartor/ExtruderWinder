#include <UI/components/settingsTab.h>
#include "Commons/globals.h"

#define ALERT_PANEL_OPTION_NUM 1
#define ALERT_PANEL_OPTION_HEIGHT 100

lv_obj_t *alertsPanel;
lv_obj_t *separatorLine;
lv_obj_t *alertOptions[ALERT_PANEL_OPTION_NUM];
lv_obj_t *wifiPanel;

typedef struct {
  lv_obj_t **panel;
  lv_obj_t **reference_panel;
  lv_align_t align;
  lv_coord_t x_offset;
  lv_coord_t y_offset;
} panel_alignment_t;

static panel_alignment_t panel_alignments[] = {
  {&wifiPanel, &alertsPanel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10},
};

static const int panel_alignments_count = sizeof(panel_alignments) / sizeof(panel_alignment_t);

static void realignAllPanels() {
  lv_obj_update_layout(alertsPanel);
  
  for (int i = 0; i < panel_alignments_count; i++) {
    panel_alignment_t *alignment = &panel_alignments[i];
    
    if (alignment->panel && *(alignment->panel) && alignment->reference_panel && *(alignment->reference_panel)) {
      lv_obj_align_to(*(alignment->panel), *(alignment->reference_panel), alignment->align, alignment->x_offset, alignment->y_offset);
    }
  }
  
  lv_obj_update_layout(lv_obj_get_parent(alertsPanel));
}

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

static void lv_weight_spinbox_decrement_event_cb(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  if(code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    lv_spinbox_decrement(alertWeightSpinbox);

    updateWeightAlertThreshold(lv_spinbox_get_value(alertWeightSpinbox));
  }
}

static void lv_weight_spinbox_increment_event_cb(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  if(code == LV_EVENT_SHORT_CLICKED || code  == LV_EVENT_LONG_PRESSED_REPEAT) {
    lv_spinbox_increment(alertWeightSpinbox);

    updateWeightAlertThreshold(lv_spinbox_get_value(alertWeightSpinbox));
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

static void alertPanelSwitch_cb(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_VALUE_CHANGED) {
    lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(event);

    updateAlertsEnabled(lv_obj_has_state(sw, LV_STATE_CHECKED));

    if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
      for (int i = 0; i < ALERT_PANEL_OPTION_NUM; i++) {
        lv_obj_clear_flag(alertOptions[i], LV_OBJ_FLAG_HIDDEN);
      }

      lv_obj_set_size(alertsPanel, lv_obj_get_width(alertsPanel), ALERT_PANEL_OPTION_HEIGHT * (ALERT_PANEL_OPTION_NUM + 1));
      lv_obj_clear_flag(separatorLine, LV_OBJ_FLAG_HIDDEN);
    } else {
      for (int i = 0; i < ALERT_PANEL_OPTION_NUM; i++) {
        lv_obj_add_flag(alertOptions[i], LV_OBJ_FLAG_HIDDEN);
      }

      lv_obj_set_size(alertsPanel, lv_obj_get_width(alertsPanel), ALERT_PANEL_OPTION_HEIGHT);
      lv_obj_add_flag(separatorLine, LV_OBJ_FLAG_HIDDEN);
    }

    realignAllPanels();
  }
}

static void wifiPanelSwitch_cb(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_VALUE_CHANGED) {
    lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(event);

    updateWifiEnabled(lv_obj_has_state(sw, LV_STATE_CHECKED));
  }
}

void build_settingsTab(lv_obj_t *parent) {
  lv_obj_t *settingsParent = lv_obj_create(parent);
  lv_obj_align(settingsParent, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_size(settingsParent, lv_obj_get_width(parent), lv_obj_get_height(parent));
  lv_obj_set_style_border_width(settingsParent, 0, 0);
  lv_obj_set_style_bg_color(settingsParent, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
  lv_obj_set_style_pad_all(settingsParent, 10, 0);

  lv_obj_update_layout(settingsParent);

  lv_obj_t *minSpeedParent = lv_obj_create(settingsParent);
  lv_obj_align(minSpeedParent, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_size(minSpeedParent, lv_obj_get_width(settingsParent) -30, 100);
  lv_obj_set_style_bg_color(minSpeedParent, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(minSpeedParent, 0, 0);

  lv_obj_t *minSpeedSpinboxLabel = lv_label_create(minSpeedParent);
  lv_obj_set_style_text_color(minSpeedSpinboxLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(minSpeedSpinboxLabel, LV_ALIGN_LEFT_MID, 10, 0);
  lv_obj_set_style_text_font(minSpeedSpinboxLabel, &lv_font_montserrat_28, 0);
  lv_label_set_text(minSpeedSpinboxLabel, "Velocidad Min");

  minSpeedSpinbox = lv_spinbox_create(minSpeedParent);
  lv_spinbox_set_range(minSpeedSpinbox, MIN_SPINBOX_SPEED, MAX_SPINBOX_SPEED);
  lv_spinbox_set_digit_format(minSpeedSpinbox, 5, 0);
  lv_spinbox_set_step(minSpeedSpinbox, 10000);
  lv_spinbox_step_prev(minSpeedSpinbox);
  lv_obj_set_width(minSpeedSpinbox, 120);
  lv_obj_align(minSpeedSpinbox, LV_ALIGN_RIGHT_MID, -70, 0);
  lv_obj_set_style_text_font(minSpeedSpinbox, &lv_font_montserrat_28, 0);
  lv_spinbox_set_value(minSpeedSpinbox, puller.minOutput);

  lv_obj_t *minSpeedSpinboxPlusBtn = lv_btn_create(minSpeedParent);
  lv_obj_set_size(minSpeedSpinboxPlusBtn, lv_obj_get_height(minSpeedSpinbox), lv_obj_get_height(minSpeedSpinbox));
  lv_obj_align_to(minSpeedSpinboxPlusBtn, minSpeedSpinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
  lv_obj_set_style_bg_img_src(minSpeedSpinboxPlusBtn, LV_SYMBOL_PLUS, 0);
  lv_obj_set_style_bg_color(minSpeedSpinboxPlusBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(minSpeedSpinboxPlusBtn, lv_min_spinbox_increment_event_cb, LV_EVENT_ALL,  NULL);

  lv_obj_t *minSpeedSpinboxMinusBtn = lv_btn_create(minSpeedParent);
  lv_obj_set_size(minSpeedSpinboxMinusBtn, lv_obj_get_height(minSpeedSpinbox), lv_obj_get_height(minSpeedSpinbox));
  lv_obj_align_to(minSpeedSpinboxMinusBtn, minSpeedSpinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_set_style_bg_img_src(minSpeedSpinboxMinusBtn, LV_SYMBOL_MINUS, 0);
  lv_obj_set_style_bg_color(minSpeedSpinboxMinusBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(minSpeedSpinboxMinusBtn, lv_min_spinbox_decrement_event_cb, LV_EVENT_ALL, NULL);

  

  lv_obj_t *maxSpeedParent = lv_obj_create(settingsParent);
  lv_obj_align_to(maxSpeedParent, minSpeedParent, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_set_size(maxSpeedParent, lv_obj_get_width(settingsParent) -30, 100);
  lv_obj_set_style_bg_color(maxSpeedParent, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(maxSpeedParent, 0, 0);

  lv_obj_t *maxSpeedSpinboxLabel = lv_label_create(maxSpeedParent);
  lv_obj_set_style_text_color(maxSpeedSpinboxLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(maxSpeedSpinboxLabel, "Velocidad Max");
  lv_obj_set_style_text_font(maxSpeedSpinboxLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(maxSpeedSpinboxLabel, LV_ALIGN_LEFT_MID, 10, 0);

  maxSpeedSpinbox = lv_spinbox_create(maxSpeedParent);
  lv_spinbox_set_range(maxSpeedSpinbox, MIN_SPINBOX_SPEED, MAX_SPINBOX_SPEED);
  lv_spinbox_set_digit_format(maxSpeedSpinbox, 5, 0);
  lv_spinbox_step_prev(maxSpeedSpinbox);
  lv_spinbox_set_step(maxSpeedSpinbox, 10000);
  lv_obj_set_width(maxSpeedSpinbox, 120);
  lv_obj_align(maxSpeedSpinbox, LV_ALIGN_RIGHT_MID, -70, 0);
  lv_obj_set_style_text_font(maxSpeedSpinbox, &lv_font_montserrat_28, 0);
  lv_spinbox_set_value(maxSpeedSpinbox, puller.maxOutput);

  lv_obj_t *maxSpeedSpinboxPlusBtn = lv_btn_create(maxSpeedParent);
  lv_obj_set_size(maxSpeedSpinboxPlusBtn, lv_obj_get_height(maxSpeedSpinbox), lv_obj_get_height(maxSpeedSpinbox));
  lv_obj_align_to(maxSpeedSpinboxPlusBtn, maxSpeedSpinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
  lv_obj_set_style_bg_img_src(maxSpeedSpinboxPlusBtn, LV_SYMBOL_PLUS, 0);
  lv_obj_set_style_bg_color(maxSpeedSpinboxPlusBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(maxSpeedSpinboxPlusBtn, lv_max_spinbox_increment_event_cb, LV_EVENT_ALL,  NULL);

  lv_obj_t *maxSpeedSpinboxMinusBtn = lv_btn_create(maxSpeedParent);
  lv_obj_set_size(maxSpeedSpinboxMinusBtn, lv_obj_get_height(maxSpeedSpinbox), lv_obj_get_height(maxSpeedSpinbox));
  lv_obj_align_to(maxSpeedSpinboxMinusBtn, maxSpeedSpinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_set_style_bg_img_src(maxSpeedSpinboxMinusBtn, LV_SYMBOL_MINUS, 0);
  lv_obj_set_style_bg_color(maxSpeedSpinboxMinusBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(maxSpeedSpinboxMinusBtn, lv_max_spinbox_decrement_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *plasticTypeParent = lv_obj_create(settingsParent);
  lv_obj_align_to(plasticTypeParent, maxSpeedParent, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_set_size(plasticTypeParent, lv_obj_get_width(settingsParent) -30, 100);
  lv_obj_set_style_bg_color(plasticTypeParent, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(plasticTypeParent, 0, 0);

  lv_obj_t *plasticTypeLabel = lv_label_create(plasticTypeParent);
  lv_obj_set_style_text_color(plasticTypeLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(plasticTypeLabel, "Tipo de plastico");
  lv_obj_set_style_text_font(plasticTypeLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(plasticTypeLabel, LV_ALIGN_LEFT_MID, 10, 0);

  lv_obj_t *plasticTypeDropdown = lv_dropdown_create(plasticTypeParent);
  std::string options;
  for (size_t i = 0; i < plasticTypes.size(); ++i) {
    options += plasticTypes[i].name.c_str();
    if (i != plasticTypes.size() - 1) options += "\n";
  }
  lv_dropdown_set_options(plasticTypeDropdown, options.c_str());
  lv_obj_set_width(plasticTypeDropdown, 200);
  lv_obj_align(plasticTypeDropdown, LV_ALIGN_RIGHT_MID, -10, 0);
  lv_dropdown_set_selected(plasticTypeDropdown, selectedPlasticTypeIndex);
  lv_obj_set_style_text_font(plasticTypeDropdown, &lv_font_montserrat_28, 0);
  lv_obj_add_event_cb(plasticTypeDropdown, plastic_type_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);



  alertsPanel = lv_obj_create(settingsParent);
  lv_obj_align_to(alertsPanel, plasticTypeParent, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_set_size(alertsPanel, lv_obj_get_width(settingsParent) -30, ALERT_PANEL_OPTION_HEIGHT * (ALERT_PANEL_OPTION_NUM + 1));
  lv_obj_set_style_bg_color(alertsPanel, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(alertsPanel, 0, 0);
  lv_obj_set_style_pad_all(alertsPanel, 0, 0);

  lv_obj_update_layout(alertsPanel);

  lv_obj_t *alertMainOptionPanel = lv_obj_create(alertsPanel);
  lv_obj_set_size(alertMainOptionPanel, lv_obj_get_width(alertsPanel), ALERT_PANEL_OPTION_HEIGHT);
  lv_obj_set_style_bg_color(alertMainOptionPanel, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(alertMainOptionPanel, 0, 0);

  lv_obj_t *alertLabel = lv_label_create(alertMainOptionPanel);
  lv_obj_set_style_text_color(alertLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(alertLabel, "Alertas");
  lv_obj_set_style_text_font(alertLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(alertLabel, LV_ALIGN_LEFT_MID, 10, 0);

  lv_obj_t *alertPanelSwitch = lv_switch_create(alertMainOptionPanel);
  lv_obj_set_size(alertPanelSwitch, 90, 50);
  lv_obj_set_style_pad_all(alertPanelSwitch, -7, LV_PART_KNOB);
  lv_obj_align(alertPanelSwitch, LV_ALIGN_RIGHT_MID, -10, 0);

  lv_obj_set_style_bg_color(alertPanelSwitch, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(alertPanelSwitch, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(alertPanelSwitch, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR | LV_STATE_FOCUSED);
  lv_obj_set_style_bg_color(alertPanelSwitch, lv_palette_main(LV_PALETTE_ORANGE), LV_STATE_CHECKED);

  separatorLine = lv_obj_create(alertsPanel);
  lv_obj_set_size(separatorLine, lv_obj_get_width(alertsPanel) - 50, 1);
  lv_obj_align_to(separatorLine, alertMainOptionPanel, LV_ALIGN_OUT_BOTTOM_MID, 0, -5);
  lv_obj_set_style_bg_color(separatorLine, lv_palette_lighten(LV_PALETTE_GREY, 1), 0);
  lv_obj_set_style_border_width(separatorLine, 0, 0);

  lv_obj_t *weightAlertContainer = lv_obj_create(alertsPanel);
  lv_obj_set_size(weightAlertContainer, lv_obj_get_width(alertsPanel) - 15, ALERT_PANEL_OPTION_HEIGHT);
  lv_obj_align_to(weightAlertContainer, alertMainOptionPanel, LV_ALIGN_OUT_BOTTOM_LEFT, 15, 0);
  lv_obj_set_style_bg_color(weightAlertContainer, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(weightAlertContainer, 0, 0);

  alertOptions[0] = weightAlertContainer;

  if (alertsEnabled) {
    lv_obj_add_state(alertPanelSwitch, LV_STATE_CHECKED);
  }

  lv_obj_add_event_cb(alertPanelSwitch, alertPanelSwitch_cb, LV_EVENT_VALUE_CHANGED, NULL);

  if (!lv_obj_has_state(alertPanelSwitch, LV_STATE_CHECKED)) {
    lv_obj_add_flag(weightAlertContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(separatorLine, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_size(alertsPanel, lv_obj_get_width(alertsPanel), ALERT_PANEL_OPTION_HEIGHT);
  }

  lv_obj_t *alertWeightLabel = lv_label_create(weightAlertContainer);
  lv_obj_set_style_text_color(alertWeightLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(alertWeightLabel, LV_SYMBOL_PLAY "  Peso (g)");
  lv_obj_set_style_text_font(alertWeightLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(alertWeightLabel, LV_ALIGN_LEFT_MID, 10, 0);

  alertWeightSpinbox = lv_spinbox_create(weightAlertContainer);
  lv_spinbox_set_range(alertWeightSpinbox, 0, MAX_SPINBOX_WEIGHT);
  lv_spinbox_set_digit_format(alertWeightSpinbox, 4, 0);
  lv_spinbox_set_step(alertWeightSpinbox, 1000);
  lv_spinbox_step_prev(alertWeightSpinbox);
  lv_obj_set_width(alertWeightSpinbox, 120);
  lv_obj_set_style_text_font(alertWeightSpinbox, &lv_font_montserrat_28, 0);
  lv_obj_align(alertWeightSpinbox, LV_ALIGN_RIGHT_MID, -70, 0);

  lv_spinbox_set_value(alertWeightSpinbox, weightAlertThreshold);

  lv_obj_t *alertWeightSpinboxPlusBtn = lv_btn_create(weightAlertContainer);
  lv_obj_set_size(alertWeightSpinboxPlusBtn, lv_obj_get_height(alertWeightSpinbox), lv_obj_get_height(alertWeightSpinbox));
  lv_obj_align_to(alertWeightSpinboxPlusBtn, alertWeightSpinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
  lv_obj_set_style_bg_img_src(alertWeightSpinboxPlusBtn, LV_SYMBOL_PLUS, 0);
  lv_obj_set_style_bg_color(alertWeightSpinboxPlusBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(alertWeightSpinboxPlusBtn, lv_weight_spinbox_increment_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *alertWeightSpinboxMinusBtn = lv_btn_create(weightAlertContainer);
  lv_obj_set_size(alertWeightSpinboxMinusBtn, lv_obj_get_height(alertWeightSpinbox), lv_obj_get_height(alertWeightSpinbox));
  lv_obj_align_to(alertWeightSpinboxMinusBtn, alertWeightSpinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_set_style_bg_img_src(alertWeightSpinboxMinusBtn, LV_SYMBOL_MINUS, 0);
  lv_obj_set_style_bg_color(alertWeightSpinboxMinusBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(alertWeightSpinboxMinusBtn, lv_weight_spinbox_decrement_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_move_foreground(separatorLine);

  wifiPanel = lv_obj_create(settingsParent);
  lv_obj_set_size(wifiPanel, lv_obj_get_width(alertsPanel), ALERT_PANEL_OPTION_HEIGHT);
  lv_obj_align_to(wifiPanel, alertsPanel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_set_style_bg_color(wifiPanel, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(wifiPanel, 0, 0);

  wifiIPLabel = lv_label_create(wifiPanel);
  lv_obj_set_style_text_color(wifiIPLabel, lv_color_hex(0xFFFFFF), 0);
  lv_label_set_text(wifiIPLabel, "WiFi: Desconectado");
  lv_obj_set_style_text_font(wifiIPLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(wifiIPLabel, LV_ALIGN_LEFT_MID, 10, 0);

  lv_obj_t *wifiPanelSwitch = lv_switch_create(wifiPanel);
  lv_obj_set_size(wifiPanelSwitch, 90, 50);
  lv_obj_set_style_pad_all(wifiPanelSwitch, -7, LV_PART_KNOB);
  lv_obj_align(wifiPanelSwitch, LV_ALIGN_RIGHT_MID, -10, 0);

  lv_obj_set_style_bg_color(wifiPanelSwitch, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(wifiPanelSwitch, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(wifiPanelSwitch, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR | LV_STATE_FOCUSED);
  lv_obj_set_style_bg_color(wifiPanelSwitch, lv_palette_main(LV_PALETTE_ORANGE), LV_STATE_CHECKED);

  if (wifiEnabled) {
    lv_obj_add_state(wifiPanelSwitch, LV_STATE_CHECKED);
  }

  lv_obj_add_event_cb(wifiPanelSwitch, wifiPanelSwitch_cb, LV_EVENT_VALUE_CHANGED, NULL);
}