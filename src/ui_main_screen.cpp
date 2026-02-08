/**
 * UI Main Screen
 * Main player screen with album art, playback controls, and volume
 */

#include "ui_common.h"

// ==================== MAIN SCREEN - CLEAN SIMPLE DESIGN ====================
void createMainScreen() {
    scr_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_main, COL_BG, 0);
    lv_obj_clear_flag(scr_main, LV_OBJ_FLAG_SCROLLABLE);

    // LEFT: Album Art Area (420px → 537px) - ambient color background
    panel_art = lv_obj_create(scr_main);
    lv_obj_set_size(panel_art, SCALE(420), SCALE(480));  // 537x600
    lv_obj_set_pos(panel_art, 0, 0);
    lv_obj_set_style_bg_color(panel_art, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_radius(panel_art, 0, 0);
    lv_obj_set_style_border_width(panel_art, 0, 0);
    lv_obj_set_style_pad_all(panel_art, 0, 0);
    lv_obj_clear_flag(panel_art, LV_OBJ_FLAG_SCROLLABLE);

    // Album art image - centered
    img_album = lv_img_create(panel_art);
    lv_obj_set_size(img_album, ART_SIZE, ART_SIZE);  // ART_SIZE should be scaled if it's a pixel value
    lv_obj_center(img_album);

    // Placeholder when no art
    art_placeholder = lv_label_create(panel_art);
    lv_label_set_text(art_placeholder, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(art_placeholder, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(art_placeholder, COL_TEXT2, 0);
    lv_obj_center(art_placeholder);

    // Album name ON the album art
    lbl_album = lv_label_create(panel_art);
    lv_obj_set_width(lbl_album, SCALE(400));  // Scaled width
    lv_label_set_long_mode(lbl_album, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(lbl_album, "");
    lv_obj_set_style_text_color(lbl_album, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_album, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(lbl_album, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_album, LV_ALIGN_BOTTOM_MID, 0, SCALE(-5));

    // RIGHT: Control Panel (380px → 486px)
    panel_right = lv_obj_create(scr_main);
    lv_obj_set_size(panel_right, SCALE(380), SCALE(480));  // 486x600
    lv_obj_set_pos(panel_right, SCALE(420), 0);  // Position at 537px (420 * 1.28)
    lv_obj_set_style_bg_color(panel_right, COL_BG, 0);
    lv_obj_set_style_radius(panel_right, 0, 0);
    lv_obj_set_style_border_width(panel_right, 0, 0);
    lv_obj_set_style_pad_all(panel_right, 0, 0);
    lv_obj_clear_flag(panel_right, LV_OBJ_FLAG_SCROLLABLE);

    // ===== TOP ROW: Back | Now Playing - Device | WiFi Queue Settings =====
    static lv_style_transition_dsc_t trans_btn;
    static lv_style_prop_t trans_props[] = {LV_STYLE_TRANSFORM_SCALE_X, LV_STYLE_TRANSFORM_SCALE_Y, LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(&trans_btn, trans_props, lv_anim_path_ease_out, 150, 0, NULL);

    // Back button
    lv_obj_t* btn_back = lv_btn_create(panel_right);
    lv_obj_set_size(btn_back, SCALE(40), SCALE(40));
    lv_obj_set_pos(btn_back, SCALE(15), SCALE(15));
    lv_obj_set_style_bg_opa(btn_back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(btn_back, 0, 0);
    lv_obj_set_style_transform_scale_x(btn_back, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(btn_back, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_back, &trans_btn, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_back, &trans_btn, 0);
    lv_obj_add_event_cb(btn_back, ev_devices, LV_EVENT_CLICKED, NULL);
    lv_obj_t* ico_back = lv_label_create(btn_back);
    lv_label_set_text(ico_back, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(ico_back, COL_TEXT, 0);
    lv_obj_center(ico_back);

    // "Now Playing - Device" label
    lbl_device_name = lv_label_create(panel_right);
    lv_label_set_text(lbl_device_name, "Now Playing");
    lv_obj_set_style_text_color(lbl_device_name, COL_TEXT2, 0);
    lv_obj_set_style_text_font(lbl_device_name, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_device_name, SCALE(60), SCALE(25));

    // Music Sources button
    lv_obj_t* btn_sources = lv_btn_create(panel_right);
    lv_obj_set_size(btn_sources, SCALE(38), SCALE(38));
    lv_obj_set_pos(btn_sources, SCALE(285), SCALE(18));
    lv_obj_set_style_bg_opa(btn_sources, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(btn_sources, 0, 0);
    lv_obj_set_style_transform_scale_x(btn_sources, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(btn_sources, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_sources, &trans_btn, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_sources, &trans_btn, 0);
    lv_obj_add_event_cb(btn_sources, [](lv_event_t* e) { lv_screen_load(scr_sources); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* ico_src = lv_label_create(btn_sources);
    lv_label_set_text(ico_src, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(ico_src, COL_TEXT, 0);
    lv_obj_set_style_text_font(ico_src, &lv_font_montserrat_20, 0);
    lv_obj_center(ico_src);

    // Settings button
    lv_obj_t* btn_settings = lv_btn_create(panel_right);
    lv_obj_set_size(btn_settings, SCALE(38), SCALE(38));
    lv_obj_set_pos(btn_settings, SCALE(335), SCALE(18));
    lv_obj_set_style_bg_opa(btn_settings, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(btn_settings, 0, 0);
    lv_obj_add_event_cb(btn_settings, ev_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_t* ico_set = lv_label_create(btn_settings);
    lv_label_set_text(ico_set, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(ico_set, COL_TEXT, 0);
    lv_obj_set_style_text_font(ico_set, &lv_font_montserrat_20, 0);
    lv_obj_center(ico_set);

    // ===== TRACK INFO =====
    lbl_artist = lv_label_create(panel_right);
    lv_obj_set_pos(lbl_artist, SCALE(20), SCALE(75));
    lv_obj_set_width(lbl_artist, SCALE(300));
    lv_label_set_long_mode(lbl_artist, LV_LABEL_LONG_DOT);
    lv_label_set_text(lbl_artist, "");
    lv_obj_set_style_text_color(lbl_artist, COL_TEXT2, 0);
    lv_obj_set_style_text_font(lbl_artist, &lv_font_montserrat_16, 0);

    lbl_title = lv_label_create(panel_right);
    lv_obj_set_pos(lbl_title, SCALE(20), SCALE(100));
    lv_obj_set_width(lbl_title, SCALE(300));
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(lbl_title, "Not Playing");
    lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_28, 0);

    // Queue/Playlist button
    btn_queue = lv_btn_create(panel_right);
    lv_obj_set_size(btn_queue, SCALE(48), SCALE(48));
    lv_obj_set_pos(btn_queue, SCALE(323), SCALE(88));
    lv_obj_set_style_bg_opa(btn_queue, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(btn_queue, 0, 0);
    lv_obj_set_style_transform_scale_x(btn_queue, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(btn_queue, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_queue, &trans_btn, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_queue, &trans_btn, 0);
    lv_obj_set_ext_click_area(btn_queue, SCALE(8));
    lv_obj_add_event_cb(btn_queue, ev_queue, LV_EVENT_CLICKED, NULL);
    lv_obj_t* ico_fav = lv_label_create(btn_queue);
    lv_label_set_text(ico_fav, LV_SYMBOL_LIST);
    lv_obj_set_style_text_color(ico_fav, COL_TEXT, 0);
    lv_obj_set_style_text_font(ico_fav, &lv_font_montserrat_18, 0);
    lv_obj_center(ico_fav);

    // ===== PROGRESS BAR =====
    slider_progress = lv_slider_create(panel_right);
    lv_obj_set_pos(slider_progress, SCALE(20), SCALE(160));
    lv_obj_set_size(slider_progress, SCALE(340), SCALE(5));
    lv_slider_set_range(slider_progress, 0, 100);
    lv_obj_set_style_bg_color(slider_progress, COL_BTN, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_progress, COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_progress, COL_ACCENT, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider_progress, 0, LV_PART_KNOB);
    lv_obj_add_event_cb(slider_progress, ev_progress, LV_EVENT_ALL, NULL);

    // Time labels
    lbl_time = lv_label_create(panel_right);
    lv_obj_set_pos(lbl_time, SCALE(20), SCALE(175));
    lv_label_set_text(lbl_time, "00:00");
    lv_obj_set_style_text_color(lbl_time, COL_TEXT2, 0);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_14, 0);

    lbl_time_remaining = lv_label_create(panel_right);
    lv_obj_set_pos(lbl_time_remaining, SCALE(315), SCALE(175));
    lv_label_set_text(lbl_time_remaining, "0:00");
    lv_obj_set_style_text_color(lbl_time_remaining, COL_TEXT2, 0);
    lv_obj_set_style_text_font(lbl_time_remaining, &lv_font_montserrat_14, 0);

    // ===== PLAYBACK CONTROLS =====
    int ctrl_y = SCALE(260);
    int center_x = SCALE(190);  // Center of 380px panel scaled

    // PLAY button
    btn_play = lv_btn_create(panel_right);
    lv_obj_set_size(btn_play, SCALE(80), SCALE(80));
    lv_obj_set_pos(btn_play, center_x - SCALE(40), ctrl_y - SCALE(40));
    lv_obj_set_style_bg_color(btn_play, COL_TEXT, 0);
    lv_obj_set_style_radius(btn_play, SCALE(40), 0);
    lv_obj_set_style_shadow_width(btn_play, 0, 0);
    lv_obj_set_style_transform_scale_x(btn_play, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(btn_play, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_play, &trans_btn, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_play, &trans_btn, 0);
    lv_obj_add_event_cb(btn_play, ev_play, LV_EVENT_CLICKED, NULL);
    lv_obj_t* ico_play = lv_label_create(btn_play);
    lv_label_set_text(ico_play, LV_SYMBOL_PAUSE);
    lv_obj_set_style_text_color(ico_play, COL_BG, 0);
    lv_obj_set_style_text_font(ico_play, &lv_font_montserrat_32, 0);
    lv_obj_center(ico_play);

    // PREV button
    btn_prev = lv_btn_create(panel_right);
    lv_obj_set_size(btn_prev, SCALE(50), SCALE(50));
    lv_obj_set_pos(btn_prev, center_x - SCALE(100), ctrl_y - SCALE(25));
    lv_obj_set_style_bg_opa(btn_prev, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(btn_prev, SCALE(25), 0);
    lv_obj_set_style_shadow_width(btn_prev, 0, 0);
    lv_obj_set_style_transform_scale_x(btn_prev, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(btn_prev, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_prev, &trans_btn, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_prev, &trans_btn, 0);
    lv_obj_add_event_cb(btn_prev, ev_prev, LV_EVENT_CLICKED, NULL);
    lv_obj_t* ico_prev = lv_label_create(btn_prev);
    lv_label_set_text(ico_prev, LV_SYMBOL_PREV);
    lv_obj_set_style_text_color(ico_prev, COL_TEXT, 0);
    lv_obj_set_style_text_font(ico_prev, &lv_font_montserrat_24, 0);
    lv_obj_center(ico_prev);

    // NEXT button
    btn_next = lv_btn_create(panel_right);
    lv_obj_set_size(btn_next, SCALE(50), SCALE(50));
    lv_obj_set_pos(btn_next, center_x + SCALE(50), ctrl_y - SCALE(25));
    lv_obj_set_style_bg_opa(btn_next, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(btn_next, SCALE(25), 0);
    lv_obj_set_style_shadow_width(btn_next, 0, 0);
    lv_obj_set_style_transform_scale_x(btn_next, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(btn_next, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_next, &trans_btn, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_next, &trans_btn, 0);
    lv_obj_add_event_cb(btn_next, ev_next, LV_EVENT_CLICKED, NULL);
    lv_obj_t* ico_next = lv_label_create(btn_next);
    lv_label_set_text(ico_next, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(ico_next, COL_TEXT, 0);
    lv_obj_set_style_text_font(ico_next, &lv_font_montserrat_24, 0);
    lv_obj_center(ico_next);

    // SHUFFLE button
    btn_shuffle = lv_btn_create(panel_right);
    lv_obj_set_size(btn_shuffle, SCALE(45), SCALE(45));
    lv_obj_set_pos(btn_shuffle, center_x - SCALE(160), ctrl_y - SCALE(22));
    lv_obj_set_style_bg_opa(btn_shuffle, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(btn_shuffle, SCALE(22), 0);
    lv_obj_set_style_shadow_width(btn_shuffle, 0, 0);
    lv_obj_set_style_transform_scale_x(btn_shuffle, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(btn_shuffle, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_shuffle, &trans_btn, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_shuffle, &trans_btn, 0);
    lv_obj_add_event_cb(btn_shuffle, ev_shuffle, LV_EVENT_CLICKED, NULL);
    lv_obj_t* ico_shuf = lv_label_create(btn_shuffle);
    lv_label_set_text(ico_shuf, LV_SYMBOL_SHUFFLE);
    lv_obj_set_style_text_color(ico_shuf, COL_TEXT2, 0);
    lv_obj_set_style_text_font(ico_shuf, &lv_font_montserrat_20, 0);
    lv_obj_center(ico_shuf);

    // REPEAT button
    btn_repeat = lv_btn_create(panel_right);
    lv_obj_set_size(btn_repeat, SCALE(45), SCALE(45));
    lv_obj_set_pos(btn_repeat, center_x + SCALE(115), ctrl_y - SCALE(22));
    lv_obj_set_style_bg_opa(btn_repeat, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(btn_repeat, SCALE(22), 0);
    lv_obj_set_style_shadow_width(btn_repeat, 0, 0);
    lv_obj_set_style_transform_scale_x(btn_repeat, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(btn_repeat, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_repeat, &trans_btn, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_repeat, &trans_btn, 0);
    lv_obj_add_event_cb(btn_repeat, ev_repeat, LV_EVENT_CLICKED, NULL);
    lv_obj_t* ico_rpt = lv_label_create(btn_repeat);
    lv_label_set_text(ico_rpt, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_color(ico_rpt, COL_TEXT2, 0);
    lv_obj_set_style_text_font(ico_rpt, &lv_font_montserrat_20, 0);
    lv_obj_center(ico_rpt);

    // ===== VOLUME SLIDER =====
    int vol_y = SCALE(340);

    // Mute button
    btn_mute = lv_btn_create(panel_right);
    lv_obj_set_size(btn_mute, SCALE(40), SCALE(40));
    lv_obj_set_pos(btn_mute, SCALE(20), vol_y);
    lv_obj_set_style_bg_opa(btn_mute, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(btn_mute, SCALE(20), 0);
    lv_obj_set_style_shadow_width(btn_mute, 0, 0);
    lv_obj_set_style_transform_scale_x(btn_mute, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale_y(btn_mute, 280, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_mute, &trans_btn, LV_STATE_PRESSED);
    lv_obj_set_style_transition(btn_mute, &trans_btn, 0);
    lv_obj_add_event_cb(btn_mute, ev_mute, LV_EVENT_CLICKED, NULL);
    lv_obj_t* ico_mute = lv_label_create(btn_mute);
    lv_label_set_text(ico_mute, LV_SYMBOL_VOLUME_MID);
    lv_obj_set_style_text_color(ico_mute, COL_TEXT2, 0);
    lv_obj_set_style_text_font(ico_mute, &lv_font_montserrat_18, 0);
    lv_obj_center(ico_mute);

    // Volume slider
    slider_vol = lv_slider_create(panel_right);
    lv_obj_set_size(slider_vol, SCALE(260), SCALE(6));
    lv_obj_set_pos(slider_vol, SCALE(70), vol_y + SCALE(17));
    lv_slider_set_range(slider_vol, 0, 100);
    lv_obj_set_style_bg_color(slider_vol, COL_BTN, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_vol, COL_TEXT2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_vol, COL_TEXT, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider_vol, 4, LV_PART_KNOB);
    lv_obj_add_event_cb(slider_vol, ev_vol_slider, LV_EVENT_ALL, NULL);

    // ===== PLAY NEXT SECTION =====
    int next_y = SCALE(440);

    // Small album art for next track
    img_next_album = lv_img_create(panel_right);
    lv_obj_set_pos(img_next_album, SCALE(20), next_y);
    lv_obj_set_size(img_next_album, SCALE(40), SCALE(40));
    lv_obj_set_style_radius(img_next_album, SCALE(4), 0);
    lv_obj_set_style_clip_corner(img_next_album, true, 0);
    lv_obj_add_flag(img_next_album, LV_OBJ_FLAG_HIDDEN);

    // "Next:" label
    lbl_next_header = lv_label_create(panel_right);
    lv_obj_set_pos(lbl_next_header, SCALE(20), next_y);
    lv_label_set_text(lbl_next_header, "Next:");
    lv_obj_set_style_text_color(lbl_next_header, COL_TEXT2, 0);
    lv_obj_set_style_text_font(lbl_next_header, &lv_font_montserrat_12, 0);

    // Next track title
    lbl_next_title = lv_label_create(panel_right);
    lv_obj_set_pos(lbl_next_title, SCALE(60), next_y);
    lv_label_set_text(lbl_next_title, "");
    lv_obj_set_style_text_color(lbl_next_title, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_next_title, &lv_font_montserrat_14, 0);
    lv_obj_set_width(lbl_next_title, SCALE(300));
    lv_label_set_long_mode(lbl_next_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_add_flag(lbl_next_title, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lbl_next_title, [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            sonos.next();
        }
    }, LV_EVENT_ALL, NULL);

    // Next track artist
    lbl_next_artist = lv_label_create(panel_right);
    lv_obj_set_pos(lbl_next_artist, SCALE(60), next_y + SCALE(18));
    lv_label_set_text(lbl_next_artist, "");
    lv_obj_set_style_text_color(lbl_next_artist, COL_TEXT2, 0);
    lv_obj_set_style_text_font(lbl_next_artist, &lv_font_montserrat_12, 0);
    lv_obj_set_width(lbl_next_artist, SCALE(300));
    lv_label_set_long_mode(lbl_next_artist, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_add_flag(lbl_next_artist, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lbl_next_artist, [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            sonos.next();
        }
    }, LV_EVENT_ALL, NULL);
}
