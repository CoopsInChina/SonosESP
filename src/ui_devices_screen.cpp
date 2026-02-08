/**
 * Devices (Speakers) Settings Screen
 * Shows discovered Sonos devices, group status, and scan functionality
 */

#include "ui_common.h"

// Forward declaration
lv_obj_t* createSettingsSidebar(lv_obj_t* screen, int activeIdx);

// ============================================================================
// Devices (Speakers) Screen
// ============================================================================
void refreshDeviceList() {
    lv_obj_clean(list_devices);
    int cnt = sonos.getDeviceCount();
    SonosDevice* current = sonos.getCurrentDevice();

    for (int i = 0; i < cnt; i++) {
        SonosDevice* dev = sonos.getDevice(i);
        if (!dev) continue;
        if (!dev->isGroupCoordinator) continue;

        // Count group members (existing logic unchanged)
        int memberCount = 1;
        for (int j = 0; j < cnt; j++) {
            if (j == i) continue;
            SonosDevice* member = sonos.getDevice(j);
            if (member && member->groupCoordinatorUUID == dev->rinconID) {
                memberCount++;
            }
        }

        bool isSelected = (current && dev->ip == current->ip);
        bool isPlaying = dev->isPlaying;
        bool hasGroup = (memberCount > 1);

        // Create main button - use SCALE() macro like in main screen
        lv_obj_t* btn = lv_btn_create(list_devices);
        
        // Calculate dimensions first to avoid complex expressions
        int btn_width = SCALE(720);
        int btn_height;
        if (hasGroup || isPlaying) {
            btn_height = SCALE(70);  // Use SCALE for both dimensions
        } else {
            btn_height = SCALE(60);
        }
        
        lv_obj_set_size(btn, btn_width, btn_height);
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        
        int btn_radius = SCALE(12);  // Scale corner radius
        lv_obj_set_style_radius(btn, btn_radius, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        
        int btn_pad = SCALE(12);  // Scale padding
        lv_obj_set_style_pad_all(btn, btn_pad, 0);

        lv_obj_set_style_bg_color(btn, isSelected ? COL_SELECTED : COL_CARD, 0);
        lv_obj_set_style_bg_color(btn, COL_BTN_PRESSED, LV_STATE_PRESSED);

        if (isSelected) {
            int border_width = SCALE(2);
            lv_obj_set_style_border_width(btn, border_width, 0);
            lv_obj_set_style_border_color(btn, COL_ACCENT, 0);
        } else {
            lv_obj_set_style_border_width(btn, 0, 0);
        }

        // Speaker icon
        lv_obj_t* icon = lv_label_create(btn);
        if (hasGroup) {
            lv_label_set_text(icon, LV_SYMBOL_AUDIO LV_SYMBOL_AUDIO);
        } else {
            lv_label_set_text(icon, LV_SYMBOL_AUDIO);
        }
        lv_obj_set_style_text_color(icon, isPlaying ? COL_ACCENT : (isSelected ? COL_ACCENT : COL_TEXT2), 0);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_18, 0);
        
        // Calculate positions first
        int icon_x = SCALE(5);
        int icon_y = (hasGroup || isPlaying) ? SCALE(-8) : 0;
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, icon_x, icon_y);

        // Room name
        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, dev->roomName.c_str());
        lv_obj_set_style_text_color(lbl, COL_TEXT, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        
        int lbl_x = hasGroup ? SCALE(55) : SCALE(45);
        int lbl_y = (hasGroup || isPlaying) ? SCALE(-8) : 0;
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, lbl_x, lbl_y);

        // Subtitle: group info or playing status
        if (hasGroup || isPlaying) {
            lv_obj_t* sub = lv_label_create(btn);
            if (hasGroup && isPlaying) {
                lv_label_set_text_fmt(sub, LV_SYMBOL_PLAY " Playing  " LV_SYMBOL_AUDIO " +%d speakers", memberCount - 1);
            } else if (hasGroup) {
                lv_label_set_text_fmt(sub, LV_SYMBOL_AUDIO " +%d speaker%s", memberCount - 1, memberCount > 2 ? "s" : "");
            } else {
                lv_label_set_text(sub, LV_SYMBOL_PLAY " Playing");
            }
            lv_obj_set_style_text_color(sub, isPlaying ? lv_color_hex(0x4ECB71) : COL_TEXT2, 0);
            lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
            
            int sub_x = hasGroup ? SCALE(55) : SCALE(45);
            int sub_y = SCALE(12);
            lv_obj_align(sub, LV_ALIGN_LEFT_MID, sub_x, sub_y);
        }

        // Right arrow indicator
        lv_obj_t* arrow = lv_label_create(btn);
        lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(arrow, COL_TEXT2, 0);
        lv_obj_set_style_text_font(arrow, &lv_font_montserrat_16, 0);
        
        int arrow_x = SCALE(-5);
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, arrow_x, 0);

        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            int idx = (int)(intptr_t)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
            sonos.selectDevice(idx);
            sonos.startTasks();
            lv_screen_load(scr_main);
        }, LV_EVENT_CLICKED, NULL);

        // Show group members as indented sub-items
        if (hasGroup) {
            for (int j = 0; j < cnt; j++) {
                if (j == i) continue;
                SonosDevice* member = sonos.getDevice(j);
                if (!member || member->groupCoordinatorUUID != dev->rinconID) continue;

                lv_obj_t* memBtn = lv_btn_create(list_devices);
                int mem_width = SCALE(684);  // 95% of 720
                int mem_height = SCALE(50);
                lv_obj_set_size(memBtn, mem_width, mem_height);
                lv_obj_set_user_data(memBtn, (void*)(intptr_t)j);
                
                int mem_radius = SCALE(8);
                lv_obj_set_style_radius(memBtn, mem_radius, 0);
                lv_obj_set_style_shadow_width(memBtn, 0, 0);
                
                int mem_pad = SCALE(10);
                lv_obj_set_style_pad_all(memBtn, mem_pad, 0);
                lv_obj_set_style_bg_color(memBtn, lv_color_hex(0x252525), 0);
                lv_obj_set_style_bg_color(memBtn, COL_BTN_PRESSED, LV_STATE_PRESSED);
                
                int mem_margin = SCALE(40);
                lv_obj_set_style_margin_left(memBtn, mem_margin, 0);

                // Linking icon
                lv_obj_t* memIcon = lv_label_create(memBtn);
                lv_label_set_text(memIcon, LV_SYMBOL_RIGHT " " LV_SYMBOL_AUDIO);
                lv_obj_set_style_text_color(memIcon, COL_TEXT2, 0);
                lv_obj_set_style_text_font(memIcon, &lv_font_montserrat_14, 0);
                
                int memIcon_x = SCALE(5);
                lv_obj_align(memIcon, LV_ALIGN_LEFT_MID, memIcon_x, 0);

                lv_obj_t* memLbl = lv_label_create(memBtn);
                lv_label_set_text(memLbl, member->roomName.c_str());
                lv_obj_set_style_text_color(memLbl, COL_TEXT, 0);
                lv_obj_set_style_text_font(memLbl, &lv_font_montserrat_16, 0);
                
                int memLbl_x = SCALE(55);
                lv_obj_align(memLbl, LV_ALIGN_LEFT_MID, memLbl_x, 0);

                // "Grouped" badge
                lv_obj_t* badge = lv_label_create(memBtn);
                lv_label_set_text(badge, "Grouped");
                lv_obj_set_style_text_color(badge, COL_TEXT2, 0);
                lv_obj_set_style_text_font(badge, &lv_font_montserrat_12, 0);
                
                int badge_x = SCALE(-10);
                lv_obj_align(badge, LV_ALIGN_RIGHT_MID, badge_x, 0);

                lv_obj_add_event_cb(memBtn, [](lv_event_t* e) {
                    int idx = (int)(intptr_t)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
                    sonos.selectDevice(idx);
                    sonos.startTasks();
                    lv_screen_load(scr_main);
                }, LV_EVENT_CLICKED, NULL);
            }
        }
    }

    // Second pass: Show any standalone non-coordinators
    for (int i = 0; i < cnt; i++) {
        SonosDevice* dev = sonos.getDevice(i);
        if (!dev || dev->isGroupCoordinator) continue;

        // Check if this device's coordinator is in our list
        bool coordinatorFound = false;
        for (int j = 0; j < cnt; j++) {
            SonosDevice* coord = sonos.getDevice(j);
            if (coord && coord->rinconID == dev->groupCoordinatorUUID) {
                coordinatorFound = true;
                break;
            }
        }

        // If coordinator not found, show as standalone
        if (!coordinatorFound) {
            bool isSelected = (current && dev->ip == current->ip);

            lv_obj_t* btn = lv_btn_create(list_devices);
            int btn_width = SCALE(720);
            int btn_height = SCALE(60);
            lv_obj_set_size(btn, btn_width, btn_height);
            lv_obj_set_user_data(btn, (void*)(intptr_t)i);
            
            int btn_radius = SCALE(12);
            lv_obj_set_style_radius(btn, btn_radius, 0);
            lv_obj_set_style_shadow_width(btn, 0, 0);
            
            int btn_pad = SCALE(15);
            lv_obj_set_style_pad_all(btn, btn_pad, 0);
            lv_obj_set_style_bg_color(btn, isSelected ? COL_SELECTED : COL_CARD, 0);

            lv_obj_t* icon = lv_label_create(btn);
            lv_label_set_text(icon, LV_SYMBOL_AUDIO);
            lv_obj_set_style_text_color(icon, COL_TEXT2, 0);
            lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
            
            int icon_x = SCALE(5);
            lv_obj_align(icon, LV_ALIGN_LEFT_MID, icon_x, 0);

            lv_obj_t* lbl = lv_label_create(btn);
            lv_label_set_text(lbl, dev->roomName.c_str());
            lv_obj_set_style_text_color(lbl, COL_TEXT, 0);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
            
            int lbl_x = SCALE(40);
            lv_obj_align(lbl, LV_ALIGN_LEFT_MID, lbl_x, 0);

            lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                int idx = (int)(intptr_t)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
                sonos.selectDevice(idx);
                sonos.startTasks();
                lv_screen_load(scr_main);
            }, LV_EVENT_CLICKED, NULL);
        }
    }
}

/**
 * Devices (Speakers) Settings Screen - Scaled for 1024x600
 */
void createDevicesScreen() {
    scr_devices = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_devices, lv_color_hex(0x121212), 0);
    lv_obj_set_size(scr_devices, SCREEN_WIDTH_TARGET, SCREEN_HEIGHT_TARGET);

    // Create sidebar and get content area (Speakers is index 0)
    lv_obj_t* content = createSettingsSidebar(scr_devices, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    
    int content_width = SCALE(720);
    lv_obj_set_size(content, content_width, SCREEN_HEIGHT_TARGET);

    // Title + Scan button row
    lv_obj_t* title_row = lv_obj_create(content);
    int title_width = SCALE(720);
    int title_height = SCALE(40);
    lv_obj_set_size(title_row, title_width, title_height);
    lv_obj_set_pos(title_row, 0, 0);
    lv_obj_set_style_bg_opa(title_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(title_row, 0, 0);
    lv_obj_set_style_pad_all(title_row, 0, 0);
    lv_obj_clear_flag(title_row, LV_OBJ_FLAG_SCROLLABLE);

    // Title label
    lv_obj_t* lbl_title = lv_label_create(title_row);
    lv_label_set_text(lbl_title, "Speakers");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 0, 0);

    // Scan button
    btn_sonos_scan = lv_btn_create(title_row);
    int btn_width = SCALE(110);
    int btn_height = SCALE(40);
    lv_obj_set_size(btn_sonos_scan, btn_width, btn_height);
    lv_obj_align(btn_sonos_scan, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_sonos_scan, COL_ACCENT, 0);
    
    int btn_radius = SCALE(20);
    lv_obj_set_style_radius(btn_sonos_scan, btn_radius, 0);
    lv_obj_set_style_shadow_width(btn_sonos_scan, 0, 0);
    lv_obj_add_event_cb(btn_sonos_scan, ev_discover, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* lbl_scan = lv_label_create(btn_sonos_scan);
    lv_label_set_text(lbl_scan, LV_SYMBOL_REFRESH " Scan");
    lv_obj_set_style_text_color(lbl_scan, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(lbl_scan, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_scan);

    // Status label
    lbl_status = lv_label_create(content);
    int status_y = SCALE(50);
    lv_obj_set_pos(lbl_status, 0, status_y);
    lv_label_set_text(lbl_status, "Tap Scan to find speakers");
    lv_obj_set_style_text_color(lbl_status, COL_TEXT2, 0);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_12, 0);

    // Devices list
    list_devices = lv_list_create(content);
    int list_width = SCALE(720);
    int list_height = SCALE(380);
    int list_y = SCALE(75);
    lv_obj_set_size(list_devices, list_width, list_height);
    lv_obj_set_pos(list_devices, 0, list_y);
    lv_obj_set_style_bg_color(list_devices, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_border_width(list_devices, 0, 0);
    lv_obj_set_style_radius(list_devices, 0, 0);
    lv_obj_set_style_pad_all(list_devices, 0, 0);
    
    int row_pad = SCALE(6);
    lv_obj_set_style_pad_row(list_devices, row_pad, 0);

    // Scrollbar styling
    int scroll_pad = SCALE(8);
    lv_obj_set_style_pad_right(list_devices, scroll_pad, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list_devices, LV_OPA_30, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(list_devices, COL_TEXT2, LV_PART_SCROLLBAR);
    
    int scroll_width = SCALE(6);
    lv_obj_set_style_width(list_devices, scroll_width, LV_PART_SCROLLBAR);
    
    int scroll_radius = SCALE(3);
    lv_obj_set_style_radius(list_devices, scroll_radius, LV_PART_SCROLLBAR);

    // Spinner for scan feedback
    spinner_scan = lv_spinner_create(content);
    int spinner_size = SCALE(100);
    lv_obj_set_size(spinner_scan, spinner_size, spinner_size);
    lv_obj_center(spinner_scan);
    lv_obj_set_style_arc_color(spinner_scan, COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner_scan, lv_color_hex(0x555555), LV_PART_MAIN);
    
    int arc_width = SCALE(10);
    lv_obj_set_style_arc_width(spinner_scan, arc_width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner_scan, arc_width, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(spinner_scan, true, LV_PART_INDICATOR);
    lv_obj_move_foreground(spinner_scan);
    lv_obj_add_flag(spinner_scan, LV_OBJ_FLAG_HIDDEN);
}