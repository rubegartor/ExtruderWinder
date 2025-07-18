#include <UI/components/controlTab.h>
#include "Commons/globals.h"

// Variables globales para elementos UI de extensiones
lv_obj_t *startExtensionLabel = nullptr;
lv_obj_t *endExtensionLabel = nullptr;

static void move_left_cb(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_LONG_PRESSED) {
    aligner.canMoveLeft = true;
  }

  if (code == LV_EVENT_RELEASED) {
    aligner.canMoveLeft = false;
  }

  if (code == LV_EVENT_SHORT_CLICKED) {
    aligner.moveTo(-aligner_to_move);
    aligner.canMoveLeft = false;
  }
}

static void move_right_cb(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_LONG_PRESSED) {
    aligner.canMoveRight = true;
  }

  if (code == LV_EVENT_RELEASED) {
    aligner.canMoveRight = false;
  }

  if (code == LV_EVENT_SHORT_CLICKED) {
    aligner.moveTo(aligner_to_move);
    aligner.canMoveRight = false;
  }
}

static void home_aligner_cb(lv_event_t *event) {
  uint32_t currentTab = lv_tabview_get_tab_active(tabview);
  // Almacenar el tab actual en el dato de usuario del tabview
  lv_obj_set_user_data(tabview, (void*)(uintptr_t)currentTab);

  setConfirmationCallback([](uint32_t originalTab) {
    aligner.resetHome();
    lv_tabview_set_act(tabview, originalTab, LV_ANIM_OFF);
  }, currentTab, "Continuar con el reset del HOME?");

  lv_tabview_set_act(tabview, 3, LV_ANIM_OFF);
}

static void start_aligner(lv_event_t *event) {
  uint32_t currentTab = lv_tabview_get_tab_active(tabview);
  // Almacenar el tab actual en el dato de usuario del tabview
  lv_obj_set_user_data(tabview, (void*)(uintptr_t)currentTab);

  setConfirmationCallback([](uint32_t originalTab) {
    aligner.startSpoolCalibration();
    lv_tabview_set_act(tabview, originalTab, LV_ANIM_OFF);
  }, currentTab, "Recalibrar la bobina?");

  lv_tabview_set_act(tabview, 3, LV_ANIM_OFF);
}

// Callbacks para extensiones de posición
static void add_start_extension_cb(lv_event_t *event) {
  aligner.addStartExtension();
  if (startExtensionLabel) {
    int32_t steps = aligner.getStartExtensionSteps();
    if (steps >= 0) {
      lv_label_set_text_fmt(startExtensionLabel, "Inicio: +%d", (int)steps);
    } else {
      lv_label_set_text_fmt(startExtensionLabel, "Inicio: %d", (int)steps);
    }
    // Realinear después de cambiar el texto
    if (moveLeftBtn) {
      lv_obj_align_to(startExtensionLabel, moveLeftBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    }
  }
}

static void remove_start_extension_cb(lv_event_t *event) {
  aligner.removeStartExtension();
  if (startExtensionLabel) {
    int32_t steps = aligner.getStartExtensionSteps();
    if (steps >= 0) {
      lv_label_set_text_fmt(startExtensionLabel, "Inicio: +%d", (int)steps);
    } else {
      lv_label_set_text_fmt(startExtensionLabel, "Inicio: %d", (int)steps);
    }
    // Realinear después de cambiar el texto
    if (moveLeftBtn) {
      lv_obj_align_to(startExtensionLabel, moveLeftBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    }
  }
}

static void add_end_extension_cb(lv_event_t *event) {
  aligner.addEndExtension();
  if (endExtensionLabel) {
    int32_t steps = aligner.getEndExtensionSteps();
    if (steps >= 0) {
      lv_label_set_text_fmt(endExtensionLabel, "Final: +%d", (int)steps);
    } else {
      lv_label_set_text_fmt(endExtensionLabel, "Final: %d", (int)steps);
    }
    // Realinear después de cambiar el texto
    if (moveRightBtn) {
      lv_obj_align_to(endExtensionLabel, moveRightBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    }
  }
}

static void remove_end_extension_cb(lv_event_t *event) {
  aligner.removeEndExtension();
  if (endExtensionLabel) {
    int32_t steps = aligner.getEndExtensionSteps();
    if (steps >= 0) {
      lv_label_set_text_fmt(endExtensionLabel, "Final: +%d", (int)steps);
    } else {
      lv_label_set_text_fmt(endExtensionLabel, "Final: %d", (int)steps);
    }
    // Realinear después de cambiar el texto
    if (moveRightBtn) {
      lv_obj_align_to(endExtensionLabel, moveRightBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    }
  }
}

void build_controlTab(lv_obj_t *parent) {
  lv_obj_t *infoSection = lv_obj_create(parent);
  lv_obj_align(infoSection, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_set_size(infoSection, lv_obj_get_width(parent) - 20, 100);
  lv_obj_set_style_bg_color(infoSection, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(infoSection, 0, 0);
  lv_obj_remove_flag(infoSection, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_update_layout(infoSection);

  static lv_style_t style_positionBar;
  lv_style_init(&style_positionBar);
  lv_style_set_bg_opa(&style_positionBar, LV_OPA_COVER);
  lv_style_set_bg_color(&style_positionBar, lv_palette_main(LV_PALETTE_ORANGE));
  lv_style_set_radius(&style_positionBar, 16);

  positionBar = lv_bar_create(infoSection);
  lv_obj_set_size(positionBar, lv_obj_get_width(infoSection) - 30, 30);
  lv_obj_align(positionBar, LV_ALIGN_CENTER, 0, -10);
  lv_bar_set_range(positionBar, 0, MAX_ALIGNER_POSITION);
  lv_bar_set_value(positionBar, 0, LV_ANIM_OFF);
  lv_obj_add_style(positionBar, &style_positionBar, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(positionBar, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_radius(positionBar, 16, 0);

  positionBarMinLabel = lv_label_create(infoSection);
  lv_label_set_text(positionBarMinLabel, "0%");
  lv_obj_set_style_text_color(positionBarMinLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(positionBarMinLabel, &lv_font_montserrat_24, 0);
  lv_obj_align(positionBarMinLabel, LV_ALIGN_BOTTOM_LEFT, 0, 5);

  positionBarMaxLabel = lv_label_create(infoSection);
  lv_label_set_text(positionBarMaxLabel, "100%");
  lv_obj_set_style_text_color(positionBarMaxLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(positionBarMaxLabel, &lv_font_montserrat_24, 0);
  lv_obj_align(positionBarMaxLabel, LV_ALIGN_BOTTOM_RIGHT, 0, 5);

  lv_obj_t *controlSection = lv_obj_create(parent);
  lv_obj_align_to(controlSection, infoSection, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_set_size(controlSection, lv_obj_get_width(parent) - 20, lv_obj_get_height(parent) - lv_obj_get_height(infoSection) - 30);
  lv_obj_set_style_bg_color(controlSection, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(controlSection, 0, 0);

  lv_obj_update_layout(controlSection);

  moveLeftBtn = lv_btn_create(controlSection);
  lv_obj_set_size(moveLeftBtn, 125, 125);
  lv_obj_align(moveLeftBtn, LV_ALIGN_CENTER, -140, -60);
  lv_obj_set_style_bg_color(moveLeftBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(moveLeftBtn, move_left_cb, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_event_cb(moveLeftBtn, move_left_cb, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(moveLeftBtn, move_left_cb, LV_EVENT_RELEASED, NULL);

  lv_obj_t *moveLeftBtnLabel = lv_label_create(moveLeftBtn);
  lv_label_set_text(moveLeftBtnLabel, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(moveLeftBtnLabel, &lv_font_montserrat_48, 0);
  lv_obj_align(moveLeftBtnLabel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(moveLeftBtnLabel, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *addLeftBtn = lv_btn_create(controlSection);
  lv_obj_set_size(addLeftBtn, 58, 58);
  lv_obj_align_to(addLeftBtn, moveLeftBtn, LV_ALIGN_OUT_LEFT_TOP, -10, 0);
  lv_obj_set_style_bg_color(addLeftBtn, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_add_event_cb(addLeftBtn, add_start_extension_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *addLeftBtnLabel = lv_label_create(controlSection);
  lv_label_set_text(addLeftBtnLabel, LV_SYMBOL_PLUS);
  lv_obj_set_style_text_font(addLeftBtnLabel, &lv_font_montserrat_32, 0);
  lv_obj_align_to(addLeftBtnLabel, addLeftBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(addLeftBtnLabel, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *removeLeftBtn = lv_btn_create(controlSection);
  lv_obj_set_size(removeLeftBtn, 58, 58);
  lv_obj_align_to(removeLeftBtn, moveLeftBtn, LV_ALIGN_OUT_LEFT_BOTTOM, -10, 0);
  lv_obj_set_style_bg_color(removeLeftBtn, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_add_event_cb(removeLeftBtn, remove_start_extension_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *removeLeftBtnLabel = lv_label_create(controlSection);
  lv_label_set_text(removeLeftBtnLabel, LV_SYMBOL_MINUS);
  lv_obj_set_style_text_font(removeLeftBtnLabel, &lv_font_montserrat_32, 0);
  lv_obj_align_to(removeLeftBtnLabel, removeLeftBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(removeLeftBtnLabel, lv_color_hex(0xFFFFFF), 0);

  homePositionBtn = lv_btn_create(controlSection);
  lv_obj_set_size(homePositionBtn, 140, 140);
  lv_obj_align(homePositionBtn, LV_ALIGN_CENTER, 0, -60);
  lv_obj_set_style_bg_color(homePositionBtn, lv_palette_main(LV_PALETTE_DEEP_ORANGE), 0);
  lv_obj_add_event_cb(homePositionBtn, home_aligner_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *homePositionBtnLabel = lv_label_create(homePositionBtn);
  lv_label_set_text(homePositionBtnLabel, LV_SYMBOL_HOME);
  lv_obj_set_style_text_font(homePositionBtnLabel, &lv_font_montserrat_48, 0);
  lv_obj_align(homePositionBtnLabel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(homePositionBtnLabel, lv_color_hex(0xFFFFFF), 0);

  moveRightBtn = lv_btn_create(controlSection);
  lv_obj_set_size(moveRightBtn, 125, 125);
  lv_obj_align(moveRightBtn, LV_ALIGN_CENTER, 140, -60);
  lv_obj_set_style_bg_color(moveRightBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(moveRightBtn, move_right_cb, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_event_cb(moveRightBtn, move_right_cb, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(moveRightBtn, move_right_cb, LV_EVENT_RELEASED, NULL);

  lv_obj_t *moveRightBtnLabel = lv_label_create(moveRightBtn);
  lv_label_set_text(moveRightBtnLabel, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_font(moveRightBtnLabel, &lv_font_montserrat_48, 0);
  lv_obj_align(moveRightBtnLabel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(moveRightBtnLabel, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *addRightBtn = lv_btn_create(controlSection);
  lv_obj_set_size(addRightBtn, 58, 58);
  lv_obj_align_to(addRightBtn, moveRightBtn, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);
  lv_obj_set_style_bg_color(addRightBtn, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_add_event_cb(addRightBtn, add_end_extension_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *addRightBtnLabel = lv_label_create(controlSection);
  lv_label_set_text(addRightBtnLabel, LV_SYMBOL_PLUS);
  lv_obj_set_style_text_font(addRightBtnLabel, &lv_font_montserrat_32, 0);
  lv_obj_align_to(addRightBtnLabel, addRightBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(addRightBtnLabel, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *removeRightBtn = lv_btn_create(controlSection);
  lv_obj_set_size(removeRightBtn, 58, 58);
  lv_obj_align_to(removeRightBtn, moveRightBtn, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);
  lv_obj_set_style_bg_color(removeRightBtn, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_add_event_cb(removeRightBtn, remove_end_extension_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *removeRightBtnLabel = lv_label_create(controlSection);
  lv_label_set_text(removeRightBtnLabel, LV_SYMBOL_MINUS);
  lv_obj_set_style_text_font(removeRightBtnLabel, &lv_font_montserrat_32, 0);
  lv_obj_align_to(removeRightBtnLabel, removeRightBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(removeRightBtnLabel, lv_color_hex(0xFFFFFF), 0);

  startExtensionLabel = lv_label_create(controlSection);
  lv_label_set_text(startExtensionLabel, "Inicio: 0");
  lv_obj_set_style_text_font(startExtensionLabel, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(startExtensionLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align_to(startExtensionLabel, moveLeftBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  endExtensionLabel = lv_label_create(controlSection);
  lv_label_set_text(endExtensionLabel, "Final: 0");
  lv_obj_set_style_text_font(endExtensionLabel, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(endExtensionLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align_to(endExtensionLabel, moveRightBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  startSpoolCalibrationBtn = lv_btn_create(controlSection);
  lv_obj_set_size(startSpoolCalibrationBtn, 280, 80);
  lv_obj_align(startSpoolCalibrationBtn, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_bg_color(startSpoolCalibrationBtn, lv_palette_darken(LV_PALETTE_BLUE, 2), 0);
  lv_obj_set_style_shadow_width(startSpoolCalibrationBtn, 0, 0);
  lv_obj_add_event_cb(startSpoolCalibrationBtn, start_aligner, LV_EVENT_CLICKED, NULL);

  lv_obj_t *startSpoolCalibrationBtnLabel = lv_label_create(startSpoolCalibrationBtn);
  lv_label_set_text(startSpoolCalibrationBtnLabel, "Calibrar bobina");
  lv_obj_set_style_text_font(startSpoolCalibrationBtnLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(startSpoolCalibrationBtnLabel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(startSpoolCalibrationBtnLabel, lv_color_hex(0xFFFFFF), 0);
}