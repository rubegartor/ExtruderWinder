#include <UI/components/sidebar.h>
#include <UI/components/homeTab.h>
#include <UI/components/popupTab.h>
#include <UI/components/general.h>

lv_obj_t *homeBtn, *listBtn, *settingsBtn;


static void open_home_tab(lv_event_t *event) {
  lv_obj_t * target = (lv_obj_t *)lv_event_get_target(event);

  lv_obj_t *label = lv_obj_get_child(target, 0);
  lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_ORANGE), 0);

  lv_obj_set_style_text_color(lv_obj_get_child(listBtn, 0), lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_color(lv_obj_get_child(settingsBtn, 0), lv_color_hex(0xFFFFFF), 0);

  lv_tabview_set_act(tabview, 0, LV_ANIM_OFF);
}

static void open_list_tab(lv_event_t *event) {
  lv_obj_t * target = (lv_obj_t *)lv_event_get_target(event);

  lv_obj_t *label = lv_obj_get_child(target, 0);
  lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_ORANGE), 0);

  lv_obj_set_style_text_color(lv_obj_get_child(homeBtn, 0), lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_color(lv_obj_get_child(settingsBtn, 0), lv_color_hex(0xFFFFFF), 0);

  lv_tabview_set_act(tabview, 1, LV_ANIM_OFF);
}

static void open_settings_tab(lv_event_t *event) {
  lv_obj_t * target = (lv_obj_t *)lv_event_get_target(event);

  lv_obj_t *label = lv_obj_get_child(target, 0);
  lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_ORANGE), 0);

  lv_obj_set_style_text_color(lv_obj_get_child(homeBtn, 0), lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_color(lv_obj_get_child(listBtn, 0), lv_color_hex(0xFFFFFF), 0);

  lv_tabview_set_act(tabview, 2, LV_ANIM_OFF);
}

void build_sidebar() {
    //Tabs
    tabview = lv_tabview_create(lv_scr_act());
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_LEFT);


    lv_obj_set_style_bg_color(tabview, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_color(tabview, lv_color_hex(0xFFFFFF), 0);

    tab1 = lv_tabview_add_tab(tabview, "A");
    lv_obj_set_style_pad_all(tab1, 0, 0);

    tab2 = lv_tabview_add_tab(tabview, "B");
    lv_obj_set_style_pad_all(tab2, 0, 0);

    tab3 = lv_tabview_add_tab(tabview, "C");
    lv_obj_set_style_pad_all(tab3, 0, 0);

    tab4 = lv_tabview_add_tab(tabview, "D");
    lv_obj_set_style_pad_all(tab4, 0, 0);

    lv_tabview_set_tab_bar_size(tabview, SIDEBAR_WIDTH);

    lv_obj_clear_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);

    //Sidebar & butons
    static lv_style_t sidebarStyle;
    lv_style_init(&sidebarStyle);
    lv_style_set_bg_opa(&sidebarStyle, LV_OPA_100);
    lv_style_set_bg_color(&sidebarStyle, lv_palette_darken(LV_PALETTE_GREY, 4));

    static lv_style_t sidebarButtonLabelStyle;
    lv_style_init(&sidebarButtonLabelStyle);
    lv_style_set_text_font(&sidebarButtonLabelStyle, &lv_font_montserrat_48);
    lv_style_set_text_color(&sidebarButtonLabelStyle, lv_color_hex(0xFFFFFF));

    lv_obj_t *sidebar = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(sidebar);
    lv_obj_set_size(sidebar, SIDEBAR_WIDTH, LV_VER_RES);
    lv_obj_add_style(sidebar, &sidebarStyle, 0);

    homeBtn = lv_btn_create(sidebar);
    lv_obj_remove_style_all(homeBtn);
    lv_obj_set_size(homeBtn, SIDEBAR_WIDTH, 125);
    lv_obj_align(homeBtn, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_event_cb(homeBtn, open_home_tab, LV_EVENT_CLICKED, NULL);

    lv_obj_t *homeBtnLabel = lv_label_create(homeBtn);
    lv_label_set_text(homeBtnLabel, LV_SYMBOL_HOME);
    lv_obj_center(homeBtnLabel);
    lv_obj_add_style(homeBtnLabel, &sidebarButtonLabelStyle, 0);
    lv_obj_set_style_text_color(homeBtnLabel, lv_palette_main(LV_PALETTE_ORANGE), 0);

    listBtn = lv_btn_create(sidebar);
    lv_obj_remove_style_all(listBtn);
    lv_obj_set_size(listBtn, SIDEBAR_WIDTH, 125);
    lv_obj_align(listBtn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(listBtn, open_list_tab, LV_EVENT_CLICKED, NULL);

    lv_obj_t *listBtnLabel = lv_label_create(listBtn);
    lv_obj_set_style_text_font(listBtnLabel, &icons, 0);
    lv_label_set_text(listBtnLabel, MOVE_SYMBOL);
    lv_obj_center(listBtnLabel);
    lv_obj_add_style(listBtnLabel, &sidebarButtonLabelStyle, 0);

    settingsBtn = lv_btn_create(sidebar);
    lv_obj_remove_style_all(settingsBtn);
    lv_obj_set_size(settingsBtn, SIDEBAR_WIDTH, 125);
    lv_obj_align(settingsBtn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(settingsBtn, open_settings_tab, LV_EVENT_CLICKED, NULL);

    lv_obj_t *settingsBtnLabel = lv_label_create(settingsBtn);
    lv_label_set_text(settingsBtnLabel, LV_SYMBOL_SETTINGS);
    lv_obj_center(settingsBtnLabel);
    lv_obj_add_style(settingsBtnLabel, &sidebarButtonLabelStyle, 0);
}