/**
 * WiFi Settings Screen
 * Network scanning and connection management
 */

#include "ui_common.h"

// Forward declaration
lv_obj_t* createSettingsSidebar(lv_obj_t* screen, int activeIdx);

// ============================================================================
// WiFi Screen
// ============================================================================
void createWiFiScreen() {
    scr_wifi = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_wifi, lv_color_hex(0x121212), 0);
    lv_obj_set_size(scr_wifi, SCREEN_WIDTH_TARGET, SCREEN_HEIGHT_TARGET);

    // Create sidebar and get content area (WiFi is index 4)
    lv_obj_t* content = createSettingsSidebar(scr_wifi, 4);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    
    int content_width = SCALE(720);
    lv_obj_set_size(content, content_width, SCREEN_HEIGHT_TARGET);

    // Title + Scan button row
    lv_obj_t* title_row = lv_obj_create(content);
    int title_row_height = SCALE(40);
    lv_obj_set_size(title_row, content_width, title_row_height);
    lv_obj_set_pos(title_row, 0, 0);
    lv_obj_set_style_bg_opa(title_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(title_row, 0, 0);
    lv_obj_set_style_pad_all(title_row, 0, 0);
    lv_obj_clear_flag(title_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl_title = lv_label_create(title_row);
    lv_label_set_text(lbl_title, "WiFi");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 0, 0);

    btn_wifi_scan = lv_btn_create(title_row);
    int scan_btn_width = SCALE(90);
    int scan_btn_height = SCALE(32);
    lv_obj_set_size(btn_wifi_scan, scan_btn_width, scan_btn_height);
    lv_obj_align(btn_wifi_scan, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_wifi_scan, COL_ACCENT, 0);
    
    int scan_btn_radius = SCALE(16);
    lv_obj_set_style_radius(btn_wifi_scan, scan_btn_radius, 0);
    lv_obj_set_style_shadow_width(btn_wifi_scan, 0, 0);
    lv_obj_add_event_cb(btn_wifi_scan, ev_wifi_scan, LV_EVENT_CLICKED, NULL);
    
    lbl_scan_text = lv_label_create(btn_wifi_scan);
    lv_label_set_text(lbl_scan_text, LV_SYMBOL_REFRESH " Scan");
    lv_obj_set_style_text_color(lbl_scan_text, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(lbl_scan_text, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_scan_text);

    // Status label
    lbl_wifi_status = lv_label_create(content);
    int status_y = SCALE(50);
    lv_obj_set_pos(lbl_wifi_status, 0, status_y);
    lv_label_set_text(lbl_wifi_status, "Tap Scan to find networks");
    lv_obj_set_style_text_color(lbl_wifi_status, COL_TEXT2, 0);
    lv_obj_set_style_text_font(lbl_wifi_status, &lv_font_montserrat_12, 0);

    // WiFi list (left column)
    list_wifi = lv_list_create(content);
    int list_width = SCALE(280);
    int list_height = SCALE(360);
    int list_y = SCALE(75);
    lv_obj_set_size(list_wifi, list_width, list_height);
    lv_obj_set_pos(list_wifi, 0, list_y);
    lv_obj_set_style_bg_color(list_wifi, COL_BG, 0);
    lv_obj_set_style_border_width(list_wifi, 0, 0);
    lv_obj_set_style_radius(list_wifi, 0, 0);
    lv_obj_set_style_pad_all(list_wifi, 0, 0);
    
    int row_pad = SCALE(6);
    lv_obj_set_style_pad_row(list_wifi, row_pad, 0);

    // Password section (right column)
    lv_obj_t* pl = lv_label_create(content);
    int pass_label_x = SCALE(290);
    int pass_label_y = SCALE(75);
    lv_obj_set_pos(pl, pass_label_x, pass_label_y);
    lv_label_set_text(pl, "Password:");
    lv_obj_set_style_text_color(pl, COL_TEXT, 0);
    lv_obj_set_style_text_font(pl, &lv_font_montserrat_14, 0);

    ta_password = lv_textarea_create(content);
    int ta_width = SCALE(300);
    int ta_height = SCALE(40);
    int ta_x = SCALE(290);
    int ta_y = SCALE(100);
    lv_obj_set_size(ta_password, ta_width, ta_height);
    lv_obj_set_pos(ta_password, ta_x, ta_y);
    lv_textarea_set_password_mode(ta_password, true);
    lv_textarea_set_placeholder_text(ta_password, "Enter password");
    lv_obj_set_style_bg_color(ta_password, COL_CARD, 0);
    lv_obj_set_style_text_color(ta_password, COL_TEXT, 0);
    lv_obj_set_style_border_color(ta_password, COL_BTN, 0);
    lv_obj_add_event_cb(ta_password, [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_FOCUSED) lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_ALL, NULL);

    btn_wifi_connect = lv_btn_create(content);
    int connect_btn_width = SCALE(300);
    int connect_btn_height = SCALE(44);
    int connect_btn_x = SCALE(290);
    int connect_btn_y = SCALE(150);
    lv_obj_set_size(btn_wifi_connect, connect_btn_width, connect_btn_height);
    lv_obj_set_pos(btn_wifi_connect, connect_btn_x, connect_btn_y);
    lv_obj_set_style_bg_color(btn_wifi_connect, COL_ACCENT, 0);
    
    int connect_btn_radius = SCALE(12);
    lv_obj_set_style_radius(btn_wifi_connect, connect_btn_radius, 0);
    lv_obj_add_event_cb(btn_wifi_connect, ev_wifi_connect, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* cl = lv_label_create(btn_wifi_connect);
    lv_label_set_text(cl, "Connect");
    lv_obj_set_style_text_color(cl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(cl, &lv_font_montserrat_16, 0);
    lv_obj_center(cl);

    // Built-in LVGL keyboard with modern professional design
    kb = lv_keyboard_create(scr_wifi);
    lv_keyboard_set_textarea(kb, ta_password);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    
    int kb_width = SCALE(615);
    int kb_height = SCALE(175);
    lv_obj_set_size(kb, kb_width, kb_height);
    
    int kb_x = SCALE(90);  // Shifted right by 90px (half of sidebar width) to avoid covering it
    int kb_y = SCALE(-5);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, kb_x, kb_y);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    // Apply custom styling to match theme
    lv_obj_set_style_bg_color(kb, COL_CARD, 0);
    
    int kb_pad = SCALE(5);
    lv_obj_set_style_pad_all(kb, kb_pad, 0);
    
    int kb_radius = SCALE(10);
    lv_obj_set_style_radius(kb, kb_radius, 0);
    lv_obj_set_style_bg_color(kb, COL_BTN, LV_PART_ITEMS);
    lv_obj_set_style_text_color(kb, COL_TEXT, LV_PART_ITEMS);
    
    int kb_item_radius = SCALE(6);
    lv_obj_set_style_radius(kb, kb_item_radius, LV_PART_ITEMS);

    // Hide keyboard when OK is pressed (auto-handled by built-in keyboard)
    lv_obj_add_event_cb(kb, [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_READY) {
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
    }, LV_EVENT_ALL, NULL);
}