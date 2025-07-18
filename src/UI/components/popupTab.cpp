#include <UI/components/popupTab.h>
#include "Commons/globals.h"

static void cancel_popup(lv_event_t *event) {
  // Obtener el tab original desde el dato de usuario del tabview
  uint32_t originalTab = (uint32_t)(uintptr_t)lv_obj_get_user_data(tabview);
  lv_tabview_set_act(tabview, originalTab, LV_ANIM_OFF);
  clearConfirmationCallback(); // Limpiar el callback si se cancela
}

static void ok_popup(lv_event_t *event) {
  executeConfirmationCallback(); // Ejecutar el callback configurado (que ya maneja volver al tab)
}

void build_popupTab(lv_obj_t *parent) {
  popupInfoLabel = lv_label_create(parent);
  lv_obj_align(popupInfoLabel, LV_ALIGN_TOP_MID, 0, 140);

  lv_obj_t *btnGroup = lv_obj_create(parent);
  lv_obj_set_size(btnGroup, lv_obj_get_width(parent) / 1.25, LV_SIZE_CONTENT);
  lv_obj_align_to(btnGroup, popupInfoLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 50);
  lv_obj_set_style_bg_opa(btnGroup, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(btnGroup, 0, 0);

  lv_obj_t *btnCancel = lv_btn_create(btnGroup);
  lv_obj_align(btnCancel, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_bg_color(btnCancel, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_set_size(btnCancel, 210, 85);
  lv_obj_add_event_cb(btnCancel, cancel_popup, LV_EVENT_CLICKED, NULL);

  lv_obj_t *btnCancelLabel = lv_label_create(btnCancel);
  lv_label_set_text(btnCancelLabel, "Cancelar");
  lv_obj_align(btnCancelLabel, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t *btnOK = lv_btn_create(btnGroup);
  lv_obj_align(btnOK, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_set_style_bg_color(btnOK, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_size(btnOK, 210, 85);
  lv_obj_add_event_cb(btnOK, ok_popup, LV_EVENT_CLICKED, NULL);

  lv_obj_t *btnOKLabel = lv_label_create(btnOK);
  lv_label_set_text(btnOKLabel, "Continuar");
  lv_obj_align(btnOKLabel, LV_ALIGN_CENTER, 0, 0);
}