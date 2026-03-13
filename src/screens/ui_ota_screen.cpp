/**
 * OTA (Over-The-Air) Update Screen
 * Firmware update management with GitHub integration
 */

#include "ui_common.h"

// Forward declaration
lv_obj_t* createSettingsSidebar(lv_obj_t* screen, int activeIdx);

// ============================================================================
// OTA Update Screen
// ============================================================================
void createOTAScreen() {
    scr_ota = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_ota, lv_color_hex(0x121212), 0);
    lv_obj_set_size(scr_ota, SCREEN_WIDTH_TARGET, SCREEN_HEIGHT_TARGET);

    // Create sidebar and get content area (Update is index 7 — Clock added at 6)
    lv_obj_t* content = createSettingsSidebar(scr_ota, 7);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    
    int content_width = SCALE(720);
    lv_obj_set_size(content, content_width, SCREEN_HEIGHT_TARGET);

    // Title
    lv_obj_t* lbl_title = lv_label_create(content);
    lv_label_set_text(lbl_title, "Firmware Update");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
    lv_obj_set_pos(lbl_title, 0, 0);

    // Version info card
    lv_obj_t* card_version = lv_obj_create(content);
    int card_width = SCALE(720);
    int card_height = SCALE(100);
    int card_y = SCALE(40);
    lv_obj_set_size(card_version, card_width, card_height);
    lv_obj_set_pos(card_version, 0, card_y);
    lv_obj_set_style_bg_color(card_version, lv_color_hex(0x2A2A2A), 0);
    
    int card_radius = SCALE(12);
    lv_obj_set_style_radius(card_version, card_radius, 0);
    lv_obj_set_style_border_width(card_version, 0, 0);
    
    int card_pad = SCALE(16);
    lv_obj_set_style_pad_all(card_version, card_pad, 0);
    lv_obj_clear_flag(card_version, LV_OBJ_FLAG_SCROLLABLE);

    lbl_current_version = lv_label_create(card_version);
    lv_label_set_text_fmt(lbl_current_version, "Current: v" FIRMWARE_VERSION);
    lv_obj_set_style_text_font(lbl_current_version, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_current_version, COL_TEXT, 0);
    lv_obj_align(lbl_current_version, LV_ALIGN_TOP_LEFT, 0, 0);

    lbl_latest_version = lv_label_create(card_version);
    lv_label_set_text(lbl_latest_version, "Latest: Checking...");
    lv_obj_set_style_text_font(lbl_latest_version, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_latest_version, COL_TEXT2, 0);
    lv_obj_align(lbl_latest_version, LV_ALIGN_TOP_LEFT, 0, SCALE(30));
// Release channel selector card
lv_obj_t* card_channel = lv_obj_create(content);
int channel_card_width = lv_pct(100);              // Percentage, no scaling needed
int channel_card_height = SCALE(60);
int channel_card_y = SCALE(155);
lv_obj_set_size(card_channel, channel_card_width, channel_card_height);
lv_obj_set_pos(card_channel, 0, channel_card_y);
lv_obj_set_style_bg_color(card_channel, lv_color_hex(0x2A2A2A), 0);

int channel_card_radius = SCALE(12);
lv_obj_set_style_radius(card_channel, channel_card_radius, 0);
lv_obj_set_style_border_width(card_channel, 0, 0);

int channel_card_pad = SCALE(16);
lv_obj_set_style_pad_all(card_channel, channel_card_pad, 0);
lv_obj_clear_flag(card_channel, LV_OBJ_FLAG_SCROLLABLE);

lv_obj_t* lbl_channel = lv_label_create(card_channel);
lv_label_set_text(lbl_channel, "Release Channel:");
lv_obj_set_style_text_font(lbl_channel, &lv_font_montserrat_14, 0);
lv_obj_set_style_text_color(lbl_channel, COL_TEXT2, 0);
lv_obj_align(lbl_channel, LV_ALIGN_LEFT_MID, 0, 0);

dd_ota_channel = lv_dropdown_create(card_channel);
lv_dropdown_set_options(dd_ota_channel, "Stable\nNightly");
int dropdown_width = SCALE(150);
int dropdown_height = SCALE(40);
lv_obj_set_size(dd_ota_channel, dropdown_width, dropdown_height);
lv_obj_align(dd_ota_channel, LV_ALIGN_RIGHT_MID, 0, 0);

// Style the dropdown button (closed state)
lv_obj_set_style_bg_color(dd_ota_channel, COL_BTN, LV_PART_MAIN);
lv_obj_set_style_bg_color(dd_ota_channel, COL_BTN_PRESSED, (lv_style_selector_t)((uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_PRESSED));
lv_obj_set_style_text_color(dd_ota_channel, COL_TEXT, LV_PART_MAIN);

int dropdown_radius = SCALE(8);
lv_obj_set_style_radius(dd_ota_channel, dropdown_radius, LV_PART_MAIN);
lv_obj_set_style_border_width(dd_ota_channel, 1, LV_PART_MAIN);
lv_obj_set_style_border_color(dd_ota_channel, lv_color_hex(0x555555), LV_PART_MAIN);

int dropdown_pad_left = SCALE(12);
int dropdown_pad_right = SCALE(12);
lv_obj_set_style_pad_left(dd_ota_channel, dropdown_pad_left, LV_PART_MAIN);
lv_obj_set_style_pad_right(dd_ota_channel, dropdown_pad_right, LV_PART_MAIN);

// Style the dropdown list (opened state) - this is the key for dark theme!
lv_obj_set_style_bg_color(dd_ota_channel, lv_color_hex(0x2A2A2A), LV_PART_SELECTED);
lv_obj_set_style_bg_color(dd_ota_channel, COL_ACCENT, (lv_style_selector_t)((uint32_t)LV_PART_SELECTED | (uint32_t)LV_STATE_CHECKED));
lv_obj_set_style_text_color(dd_ota_channel, COL_TEXT, LV_PART_SELECTED);

// Get the list object and style it for dark theme
lv_obj_t* list = lv_dropdown_get_list(dd_ota_channel);
if (list) {
    lv_obj_set_style_bg_color(list, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_text_color(list, COL_TEXT, 0);
    lv_obj_set_style_border_color(list, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_width(list, 1, 0);
}

// Load saved channel preference (default to Stable=0)
ota_channel = wifiPrefs.getInt("ota_channel", 0);
lv_dropdown_set_selected(dd_ota_channel, ota_channel);

// Channel change callback
lv_obj_add_event_cb(dd_ota_channel, [](lv_event_t* e) {
    ota_channel = lv_dropdown_get_selected(dd_ota_channel);
    wifiPrefs.putInt("ota_channel", ota_channel);
    Serial.printf("[OTA] Channel changed to: %s\n", ota_channel == 0 ? "Stable" : "Nightly");
}, LV_EVENT_VALUE_CHANGED, NULL);

    // Status label
    lbl_ota_status = lv_label_create(content);
    int status_y = SCALE(230);
    lv_obj_set_pos(lbl_ota_status, 0, status_y);
    lv_label_set_text(lbl_ota_status, "Tap 'Check for Updates' to begin");
    lv_obj_set_style_text_color(lbl_ota_status, COL_TEXT2, 0);
    lv_obj_set_style_text_font(lbl_ota_status, &lv_font_montserrat_14, 0);
    lv_obj_set_width(lbl_ota_status, SCALE(720));
    lv_label_set_long_mode(lbl_ota_status, LV_LABEL_LONG_WRAP);

    // Progress label
    lbl_ota_progress = lv_label_create(content);
    int progress_y = SCALE(260);
    lv_obj_set_pos(lbl_ota_progress, 0, progress_y);
    lv_label_set_text(lbl_ota_progress, "");
    lv_obj_set_style_text_color(lbl_ota_progress, COL_ACCENT, 0);
    lv_obj_set_style_text_font(lbl_ota_progress, &lv_font_montserrat_16, 0);

    // Visual progress bar (hidden by default)
    bar_ota_progress = lv_bar_create(content);
    int progress_bar_width = lv_pct(100);  // Percentage, no scaling needed
    int progress_bar_height = SCALE(16);
    int progress_bar_y = SCALE(330);
    lv_obj_set_size(bar_ota_progress, progress_bar_width, progress_bar_height);
    lv_obj_set_pos(bar_ota_progress, 0, progress_bar_y);

    lv_bar_set_range(bar_ota_progress, 0, 100);
    lv_bar_set_value(bar_ota_progress, 0, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(bar_ota_progress, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar_ota_progress, LV_OPA_COVER, LV_PART_MAIN);  
    
    int bar_radius = SCALE(8);
    lv_obj_set_style_radius(bar_ota_progress, bar_radius, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_ota_progress, COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar_ota_progress, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_ota_progress, bar_radius, LV_PART_INDICATOR);
    lv_obj_add_flag(bar_ota_progress, LV_OBJ_FLAG_HIDDEN);

    // Check for Updates button
    btn_check_update = lv_btn_create(content);
    int btn_width = SCALE(280);
    int btn_height = SCALE(50);
    int btn_y = SCALE(330);
    lv_obj_set_size(btn_check_update, btn_width, btn_height);
    lv_obj_set_pos(btn_check_update, 0, btn_y);
    lv_obj_set_style_bg_color(btn_check_update, COL_ACCENT, 0);
    
    int btn_radius = SCALE(12);
    lv_obj_set_style_radius(btn_check_update, btn_radius, 0);
    lv_obj_add_event_cb(btn_check_update, ev_check_update, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* lbl_check = lv_label_create(btn_check_update);
    lv_label_set_text(lbl_check, LV_SYMBOL_REFRESH " Check for Updates");
    lv_obj_set_style_text_color(lbl_check, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(lbl_check, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_check);

    // Install Update button (hidden by default)
    btn_install_update = lv_btn_create(content);
    int install_btn_x = SCALE(310);
    lv_obj_set_size(btn_install_update, btn_width, btn_height);
    lv_obj_set_pos(btn_install_update, install_btn_x, btn_y);
    lv_obj_set_style_bg_color(btn_install_update, lv_color_hex(0x4ECB71), 0);
    lv_obj_set_style_radius(btn_install_update, btn_radius, 0);
    lv_obj_add_event_cb(btn_install_update, ev_install_update, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* lbl_install = lv_label_create(btn_install_update);
    lv_label_set_text(lbl_install, LV_SYMBOL_DOWNLOAD " Install Update");
    lv_obj_set_style_text_color(lbl_install, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(lbl_install, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_install);
    lv_obj_add_flag(btn_install_update, LV_OBJ_FLAG_HIDDEN);  // Hidden until update available

    // Info text
    lv_obj_t* lbl_info = lv_label_create(content);
    int info_y = SCALE(330);
    lv_obj_set_pos(lbl_info, 0, info_y);
    lv_label_set_text(lbl_info,
        LV_SYMBOL_WARNING "  Do not disconnect power during update!\n"
        "Stable: Auto-releases | Nightly: Latest test builds (may be unstable)");
    lv_obj_set_style_text_color(lbl_info, COL_TEXT2, 0);
    lv_obj_set_style_text_font(lbl_info, &lv_font_montserrat_12, 0);
    lv_obj_set_width(lbl_info, SCALE(720));
    lv_label_set_long_mode(lbl_info, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(lbl_info, 0, SCALE(400));
}