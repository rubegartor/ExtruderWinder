#include <Screen/components/controlTab.h>

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

static void add_right_cb(lv_event_t *event) {
  aligner.endPos = aligner.endPos + aligner_to_move;
  aligner_right_pos += 1;
}

static void remove_right_cb(lv_event_t *event) {
  aligner.endPos = aligner.endPos - aligner_to_move;
  aligner_right_pos -= 1;
}

static void add_left_cb(lv_event_t *event) {
  aligner.startPos = aligner.startPos + aligner_to_move;
  aligner_left_pos += 1;
}

static void remove_left_cb(lv_event_t *event) {
  aligner.startPos = aligner.startPos - aligner_to_move;
  aligner_left_pos -= 1;
}

static void home_aligner_cb(lv_event_t *event) {
  aligner.resetHome();
}

static void set_aligner_end_position(lv_event_t *event) {
  aligner.setEndPosition();

  lv_obj_t * target = (lv_obj_t *)lv_event_get_target(event);

  lv_bar_set_range(positionBar, 0, aligner.endPos);

  lv_obj_t *tempParent = lv_obj_get_parent(target);

  lv_obj_del(target);

  lv_obj_set_style_pad_top(tempParent, 80, 0);
}

static void set_aligner_start_position(lv_event_t *event) {
  aligner.setStartPosition();

  lv_obj_t * target = (lv_obj_t *)lv_event_get_target(event);

  lv_obj_t *alignerSetEndPositionBtn = lv_btn_create(lv_obj_get_parent(target));
  lv_obj_set_size(alignerSetEndPositionBtn, LV_SIZE_CONTENT, 80);
  lv_obj_align(alignerSetEndPositionBtn, LV_ALIGN_CENTER, 0, 80);
  lv_obj_add_event_cb(alignerSetEndPositionBtn, set_aligner_end_position, LV_EVENT_CLICKED, NULL);

  lv_obj_t *alignerSetEndPositionBtnLabel = lv_label_create(alignerSetEndPositionBtn);
  lv_obj_align(alignerSetEndPositionBtnLabel, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(alignerSetEndPositionBtnLabel, "Establecer fin");
  lv_obj_set_style_text_color(alignerSetEndPositionBtnLabel, lv_color_hex(0xFFFFFF), 0);

  lv_obj_del(target);
}

static void start_aligner(lv_event_t *event) {
  lv_obj_t * target = (lv_obj_t *)lv_event_get_target(event);
  lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *alignerSetStartPositionBtn = lv_btn_create(lv_obj_get_parent(target));
  lv_obj_set_size(alignerSetStartPositionBtn, LV_SIZE_CONTENT, 80);
  lv_obj_align(alignerSetStartPositionBtn, LV_ALIGN_CENTER, 0, 80);
  lv_obj_add_event_cb(alignerSetStartPositionBtn, set_aligner_start_position, LV_EVENT_CLICKED, NULL);

  lv_obj_t *alignerSetStartPositionBtnLabel = lv_label_create(alignerSetStartPositionBtn);
  lv_obj_align(alignerSetStartPositionBtnLabel, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(alignerSetStartPositionBtnLabel, "Establecer inicio");
  lv_obj_set_style_text_color(alignerSetStartPositionBtnLabel, lv_color_hex(0xFFFFFF), 0);
}

void build_controlTab(lv_obj_t *parent) {
  lv_obj_t *controlParentTop = lv_obj_create(parent);
  lv_obj_align(controlParentTop, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_set_size(controlParentTop, lv_obj_get_width(parent) - 20, 125);
  lv_obj_set_style_bg_color(controlParentTop, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(controlParentTop, 0, 0);

  static lv_style_t style_positionBar;

  lv_style_init(&style_positionBar);
  lv_style_set_bg_opa(&style_positionBar, LV_OPA_COVER);
  lv_style_set_bg_color(&style_positionBar, lv_palette_main(LV_PALETTE_ORANGE));

  positionBar = lv_bar_create(controlParentTop);
  lv_obj_set_size(positionBar, lv_obj_get_width(parent) - 80, 30);
  lv_obj_align(positionBar, LV_ALIGN_TOP_MID, 0, 10);
  lv_bar_set_range(positionBar, 0, MAX_ALIGNER_POSITION);
  lv_bar_set_value(positionBar, 0, LV_ANIM_OFF);
  lv_obj_add_style(positionBar, &style_positionBar, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(positionBar, lv_palette_lighten(LV_PALETTE_GREY, 3), 0);

  positionBarMinLabel = lv_label_create(controlParentTop);
  lv_label_set_text(positionBarMinLabel, "0% (0)");
  lv_obj_set_style_text_color(positionBarMinLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(positionBarMinLabel, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  positionBarMaxLabel = lv_label_create(controlParentTop);
  lv_label_set_text(positionBarMaxLabel, "100% (0)");
  lv_obj_set_style_text_color(positionBarMaxLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(positionBarMaxLabel, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  lv_obj_t * controlParentContent = lv_obj_create(parent);
  lv_obj_align_to(controlParentContent, controlParentTop, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_set_size(controlParentContent, lv_obj_get_width(parent) - 20, lv_obj_get_height(parent) - lv_obj_get_height(controlParentTop) - 30);
  lv_obj_set_style_bg_color(controlParentContent, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_border_width(controlParentContent, 0, 0);

  homePositionBtn = lv_btn_create(controlParentContent);
  lv_obj_set_size(homePositionBtn, 125, 125);
  lv_obj_align_to(homePositionBtn, controlParentContent, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_set_style_bg_color(homePositionBtn, lv_palette_main(LV_PALETTE_DEEP_ORANGE), 0);
  lv_obj_add_event_cb(homePositionBtn, home_aligner_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *homePositionBtnLabel = lv_label_create(homePositionBtn);
  lv_label_set_text(homePositionBtnLabel, LV_SYMBOL_HOME);
  lv_obj_set_style_text_font(homePositionBtnLabel, &lv_font_montserrat_48, 0);
  lv_obj_align(homePositionBtnLabel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(homePositionBtnLabel, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *moveLeftBtn = lv_btn_create(controlParentContent);
  lv_obj_set_size(moveLeftBtn, 125, 125);
  lv_obj_align_to(moveLeftBtn, homePositionBtn, LV_ALIGN_OUT_LEFT_MID, -10, 0);
  lv_obj_set_style_bg_color(moveLeftBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(moveLeftBtn, move_left_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *moveLeftBtnLabel = lv_label_create(controlParentContent);
  lv_label_set_text(moveLeftBtnLabel, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(moveLeftBtnLabel, &lv_font_montserrat_48, 0);
  lv_obj_align_to(moveLeftBtnLabel, moveLeftBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(moveLeftBtnLabel, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *addLeftBtn = lv_btn_create(controlParentContent);
  lv_obj_set_size(addLeftBtn, 58, 58);
  lv_obj_align_to(addLeftBtn, moveLeftBtn, LV_ALIGN_OUT_LEFT_TOP, -10, 0);
  lv_obj_set_style_bg_color(addLeftBtn, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_add_event_cb(addLeftBtn, add_left_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *addLeftBtnLabel = lv_label_create(controlParentContent);
  lv_label_set_text(addLeftBtnLabel, LV_SYMBOL_PLUS);
  lv_obj_set_style_text_font(addLeftBtnLabel, &lv_font_montserrat_32, 0);
  lv_obj_align_to(addLeftBtnLabel, addLeftBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(addLeftBtnLabel, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *removeLeftBtn = lv_btn_create(controlParentContent);
  lv_obj_set_size(removeLeftBtn, 58, 58);
  lv_obj_align_to(removeLeftBtn, moveLeftBtn, LV_ALIGN_OUT_LEFT_BOTTOM, -10, 0);
  lv_obj_set_style_bg_color(removeLeftBtn, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_add_event_cb(removeLeftBtn, remove_left_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *removeLeftBtnLabel = lv_label_create(controlParentContent);
  lv_label_set_text(removeLeftBtnLabel, LV_SYMBOL_MINUS);
  lv_obj_set_style_text_font(removeLeftBtnLabel, &lv_font_montserrat_32, 0);
  lv_obj_align_to(removeLeftBtnLabel, removeLeftBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(removeLeftBtnLabel, lv_color_hex(0xFFFFFF), 0);



  lv_obj_t *moveRightBtn = lv_btn_create(controlParentContent);
  lv_obj_set_size(moveRightBtn, 125, 125);
  lv_obj_align_to(moveRightBtn, homePositionBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  lv_obj_set_style_bg_color(moveRightBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(moveRightBtn, move_right_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *moveRightBtnLabel = lv_label_create(controlParentContent);
  lv_label_set_text(moveRightBtnLabel, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_font(moveRightBtnLabel, &lv_font_montserrat_48, 0);
  lv_obj_align_to(moveRightBtnLabel, moveRightBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(moveRightBtnLabel, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *addRightBtn = lv_btn_create(controlParentContent);
  lv_obj_set_size(addRightBtn, 58, 58);
  lv_obj_align_to(addRightBtn, moveRightBtn, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);
  lv_obj_set_style_bg_color(addRightBtn, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_add_event_cb(addRightBtn, add_right_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *addRightBtnLabel = lv_label_create(controlParentContent);
  lv_label_set_text(addRightBtnLabel, LV_SYMBOL_PLUS);
  lv_obj_set_style_text_font(addRightBtnLabel, &lv_font_montserrat_32, 0);
  lv_obj_align_to(addRightBtnLabel, addRightBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(addRightBtnLabel, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *removeRightBtn = lv_btn_create(controlParentContent);
  lv_obj_set_size(removeRightBtn, 58, 58);
  lv_obj_align_to(removeRightBtn, moveRightBtn, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);
  lv_obj_set_style_bg_color(removeRightBtn, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_add_event_cb(removeRightBtn, remove_right_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *removeRightBtnLabel = lv_label_create(controlParentContent);
  lv_label_set_text(removeRightBtnLabel, LV_SYMBOL_MINUS);
  lv_obj_set_style_text_font(removeRightBtnLabel, &lv_font_montserrat_32, 0);
  lv_obj_align_to(removeRightBtnLabel, removeRightBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(removeRightBtnLabel, lv_color_hex(0xFFFFFF), 0);

  startAlignerBtn = lv_btn_create(controlParentContent);
  lv_obj_align(startAlignerBtn, LV_ALIGN_CENTER, 0, 80);
  lv_obj_set_style_bg_color(startAlignerBtn, lv_palette_darken(LV_PALETTE_BLUE, 2), 0);
  lv_obj_set_size(startAlignerBtn, LV_SIZE_CONTENT, 80);
  lv_obj_set_style_shadow_width(startAlignerBtn, 0, 0);
  lv_obj_add_event_cb(startAlignerBtn, start_aligner, LV_EVENT_CLICKED, NULL);

  lv_obj_t *startAlignerBtnLabel = lv_label_create(startAlignerBtn);
  lv_label_set_text(startAlignerBtnLabel, "Comenzar alineamiento");
  lv_obj_set_style_text_font(startAlignerBtnLabel, &lv_font_montserrat_32, 0);
  lv_obj_align_to(startAlignerBtnLabel, startAlignerBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(startAlignerBtnLabel, lv_color_hex(0xFFFFFF), 0);
}