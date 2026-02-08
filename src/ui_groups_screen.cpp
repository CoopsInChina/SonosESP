/**
 * Groups Settings Screen
 * Manages Sonos speaker groups (join/leave operations)
 */

#include "ui_common.h"

// Forward declaration
lv_obj_t* createSettingsSidebar(lv_obj_t* screen, int activeIdx);

// ============================================================================
// Groups Screen
// ============================================================================
void refreshGroupsList() {
    if (!list_groups) return;
    lv_obj_clean(list_groups);

    int cnt = sonos.getDeviceCount();
    if (cnt == 0) {
        lv_label_set_text(lbl_groups_status, "No speakers found. Tap Scan to discover.");
        return;
    }

    // Count groups (coordinators)
    int groupCount = 0;
    for (int i = 0; i < cnt; i++) {
        SonosDevice* dev = sonos.getDevice(i);
        if (dev && dev->isGroupCoordinator) groupCount++;
    }

    lv_label_set_text_fmt(lbl_groups_status, "%d speaker%s, %d group%s",
        cnt, cnt == 1 ? "" : "s",
        groupCount, groupCount == 1 ? "" : "s");

    // First pass: Show group coordinators with their members
    for (int i = 0; i < cnt; i++) {
        SonosDevice* dev = sonos.getDevice(i);
        if (!dev || !dev->isGroupCoordinator) continue;

        // Count members in this group
        int memberCount = 0;
        for (int j = 0; j < cnt; j++) {
            SonosDevice* member = sonos.getDevice(j);
            if (member && (j == i || member->groupCoordinatorUUID == dev->rinconID)) {
                memberCount++;
            }
        }

        bool isSelected = (selected_group_coordinator == i);
        bool isPlaying = dev->isPlaying;
        bool hasTrack = (dev->currentTrack.length() > 0);

        // Create group header button - taller to show now playing info
        lv_obj_t* btn = lv_btn_create(list_groups);
        int btn_width = SCALE(720);
        int btn_height = (isPlaying && hasTrack) ? SCALE(85) : SCALE(70);
        lv_obj_set_size(btn, btn_width, btn_height);
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        
        int btn_radius = SCALE(12);
        lv_obj_set_style_radius(btn, btn_radius, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        
        int btn_pad = SCALE(12);
        lv_obj_set_style_pad_all(btn, btn_pad, 0);

        lv_obj_set_style_bg_color(btn, isSelected ? COL_SELECTED : COL_CARD, 0);
        lv_obj_set_style_bg_color(btn, COL_BTN_PRESSED, LV_STATE_PRESSED);

        if (isSelected) {
            int border_width = SCALE(2);
            lv_obj_set_style_border_width(btn, border_width, 0);
            lv_obj_set_style_border_color(btn, COL_ACCENT, 0);
        } else if (isPlaying) {
            int border_width = SCALE(2);
            lv_obj_set_style_border_width(btn, border_width, 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x4ECB71), 0);
        } else {
            lv_obj_set_style_border_width(btn, 0, 0);
        }

        // Group icon with playing indicator
        lv_obj_t* icon = lv_label_create(btn);
        if (isPlaying) {
            lv_label_set_text(icon, memberCount > 1 ? LV_SYMBOL_PLAY " " LV_SYMBOL_AUDIO LV_SYMBOL_AUDIO : LV_SYMBOL_PLAY " " LV_SYMBOL_AUDIO);
        } else {
            lv_label_set_text(icon, memberCount > 1 ? LV_SYMBOL_AUDIO LV_SYMBOL_AUDIO : LV_SYMBOL_AUDIO);
        }
        lv_obj_set_style_text_color(icon, isPlaying ? lv_color_hex(0x4ECB71) : (memberCount > 1 ? COL_ACCENT : COL_TEXT2), 0);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_16, 0);
        
        int icon_x = SCALE(5);
        int icon_y = (isPlaying && hasTrack) ? SCALE(-18) : SCALE(-8);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, icon_x, icon_y);

        // Room name (coordinator)
        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, dev->roomName.c_str());
        lv_obj_set_style_text_color(lbl, COL_TEXT, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        
        int lbl_x = isPlaying ? SCALE(70) : SCALE(55);
        int lbl_y = (isPlaying && hasTrack) ? SCALE(-18) : SCALE(-8);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, lbl_x, lbl_y);

        // Member count / status subtitle
        lv_obj_t* sub = lv_label_create(btn);
        if (memberCount > 1) {
            lv_label_set_text_fmt(sub, "%d speakers in group", memberCount);
        } else {
            lv_label_set_text(sub, "Standalone");
        }
        lv_obj_set_style_text_color(sub, COL_TEXT2, 0);
        lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
        
        int sub_x = isPlaying ? SCALE(70) : SCALE(55);
        int sub_y = (isPlaying && hasTrack) ? SCALE(2) : SCALE(12);
        lv_obj_align(sub, LV_ALIGN_LEFT_MID, sub_x, sub_y);

        // Now playing info (if playing)
        if (isPlaying && hasTrack) {
            lv_obj_t* nowPlaying = lv_label_create(btn);
            String trackInfo = dev->currentTrack;
            if (dev->currentArtist.length() > 0) {
                trackInfo += " - " + dev->currentArtist;
            }
            // Truncate if too long
            if (trackInfo.length() > 45) {
                trackInfo = trackInfo.substring(0, 42) + "...";
            }
            lv_label_set_text(nowPlaying, trackInfo.c_str());
            lv_obj_set_style_text_color(nowPlaying, lv_color_hex(0x4ECB71), 0);
            lv_obj_set_style_text_font(nowPlaying, &lv_font_montserrat_12, 0);
            
            int nowPlaying_x = SCALE(70);
            int nowPlaying_y = SCALE(22);
            lv_obj_align(nowPlaying, LV_ALIGN_LEFT_MID, nowPlaying_x, nowPlaying_y);
        }

        // Click to select this group for management
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            int idx = (int)(intptr_t)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
            selected_group_coordinator = (selected_group_coordinator == idx) ? -1 : idx;
            refreshGroupsList();
        }, LV_EVENT_CLICKED, NULL);

        // Show group members as sub-items if this group is selected
        if (isSelected && memberCount > 1) {
            for (int j = 0; j < cnt; j++) {
                if (j == i) continue;  // Skip coordinator
                SonosDevice* member = sonos.getDevice(j);
                if (!member || member->groupCoordinatorUUID != dev->rinconID) continue;

                // Member item (indented)
                lv_obj_t* memBtn = lv_btn_create(list_groups);
                int memBtn_width = SCALE(680);
                int memBtn_height = SCALE(50);
                lv_obj_set_size(memBtn, memBtn_width, memBtn_height);
                lv_obj_set_user_data(memBtn, (void*)(intptr_t)j);
                
                int memBtn_radius = SCALE(8);
                lv_obj_set_style_radius(memBtn, memBtn_radius, 0);
                lv_obj_set_style_shadow_width(memBtn, 0, 0);
                
                int memBtn_pad = SCALE(10);
                lv_obj_set_style_pad_all(memBtn, memBtn_pad, 0);
                lv_obj_set_style_bg_color(memBtn, lv_color_hex(0x252525), 0);
                lv_obj_set_style_bg_color(memBtn, COL_BTN_PRESSED, LV_STATE_PRESSED);
                
                int memBtn_margin = SCALE(40);
                lv_obj_set_style_margin_left(memBtn, memBtn_margin, 0);

                lv_obj_t* memIcon = lv_label_create(memBtn);
                lv_label_set_text(memIcon, LV_SYMBOL_RIGHT " " LV_SYMBOL_AUDIO);
                lv_obj_set_style_text_color(memIcon, COL_TEXT2, 0);
                lv_obj_set_style_text_font(memIcon, &lv_font_montserrat_16, 0);
                
                int memIcon_x = SCALE(5);
                lv_obj_align(memIcon, LV_ALIGN_LEFT_MID, memIcon_x, 0);

                lv_obj_t* memLbl = lv_label_create(memBtn);
                lv_label_set_text(memLbl, member->roomName.c_str());
                lv_obj_set_style_text_color(memLbl, COL_TEXT, 0);
                lv_obj_set_style_text_font(memLbl, &lv_font_montserrat_16, 0);
                
                int memLbl_x = SCALE(60);
                lv_obj_align(memLbl, LV_ALIGN_LEFT_MID, memLbl_x, 0);

                // Remove from group button
                lv_obj_t* removeBtn = lv_btn_create(memBtn);
                int removeBtn_width = SCALE(90);
                int removeBtn_height = SCALE(35);
                lv_obj_set_size(removeBtn, removeBtn_width, removeBtn_height);
                lv_obj_align(removeBtn, LV_ALIGN_RIGHT_MID, SCALE(-5), 0);
                lv_obj_set_style_bg_color(removeBtn, lv_color_hex(0x8B0000), 0);
                
                int removeBtn_radius = SCALE(8);
                lv_obj_set_style_radius(removeBtn, removeBtn_radius, 0);
                lv_obj_set_user_data(removeBtn, (void*)(intptr_t)j);

                lv_obj_t* removeLbl = lv_label_create(removeBtn);
                lv_label_set_text(removeLbl, "Remove");
                lv_obj_set_style_text_color(removeLbl, COL_TEXT, 0);
                lv_obj_set_style_text_font(removeLbl, &lv_font_montserrat_14, 0);
                lv_obj_center(removeLbl);

                lv_obj_add_event_cb(removeBtn, [](lv_event_t* e) {
                    lv_event_stop_bubbling(e);
                    int idx = (int)(intptr_t)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
                    sonos.leaveGroup(idx);
                    lv_label_set_text(lbl_groups_status, "Removing from group...");
                    lv_timer_handler();
                    vTaskDelay(pdMS_TO_TICKS(500));
                    sonos.updateGroupInfo();
                    refreshGroupsList();
                }, LV_EVENT_CLICKED, NULL);
            }
        }
    }

    // If a group is selected, show standalone speakers that can be added
    if (selected_group_coordinator >= 0) {
        SonosDevice* coordinator = sonos.getDevice(selected_group_coordinator);
        if (coordinator) {
            // Header for available speakers
            lv_obj_t* hdr = lv_obj_create(list_groups);
            int hdr_width = SCALE(720);
            int hdr_height = SCALE(40);
            lv_obj_set_size(hdr, hdr_width, hdr_height);
            lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1A1A1A), 0);
            lv_obj_set_style_border_width(hdr, 0, 0);
            
            int hdr_pad = SCALE(10);
            lv_obj_set_style_pad_all(hdr, hdr_pad, 0);
            lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t* hdrLbl = lv_label_create(hdr);
            lv_label_set_text_fmt(hdrLbl, "Add speakers to \"%s\":", coordinator->roomName.c_str());
            lv_obj_set_style_text_color(hdrLbl, COL_ACCENT, 0);
            lv_obj_set_style_text_font(hdrLbl, &lv_font_montserrat_16, 0);
            lv_obj_align(hdrLbl, LV_ALIGN_LEFT_MID, 0, 0);

            // Show standalone speakers (not in any group except their own)
            for (int i = 0; i < cnt; i++) {
                if (i == selected_group_coordinator) continue;
                SonosDevice* dev = sonos.getDevice(i);
                if (!dev) continue;

                // Skip if already in the selected group
                if (dev->groupCoordinatorUUID == coordinator->rinconID) continue;

                // Only show if standalone (is own coordinator)
                if (!dev->isGroupCoordinator) continue;

                lv_obj_t* addBtn = lv_btn_create(list_groups);
                int addBtn_width = SCALE(720);
                int addBtn_height = SCALE(55);
                lv_obj_set_size(addBtn, addBtn_width, addBtn_height);
                lv_obj_set_user_data(addBtn, (void*)(intptr_t)i);
                
                int addBtn_radius = SCALE(10);
                lv_obj_set_style_radius(addBtn, addBtn_radius, 0);
                lv_obj_set_style_shadow_width(addBtn, 0, 0);
                
                int addBtn_pad = SCALE(10);
                lv_obj_set_style_pad_all(addBtn, addBtn_pad, 0);
                lv_obj_set_style_bg_color(addBtn, lv_color_hex(0x1E3A1E), 0);  // Dark green hint
                lv_obj_set_style_bg_color(addBtn, lv_color_hex(0x2A5A2A), LV_STATE_PRESSED);

                lv_obj_t* addIcon = lv_label_create(addBtn);
                lv_label_set_text(addIcon, LV_SYMBOL_PLUS " " LV_SYMBOL_AUDIO);
                lv_obj_set_style_text_color(addIcon, lv_color_hex(0x4ECB71), 0);
                lv_obj_set_style_text_font(addIcon, &lv_font_montserrat_18, 0);
                
                int addIcon_x = SCALE(5);
                lv_obj_align(addIcon, LV_ALIGN_LEFT_MID, addIcon_x, 0);

                lv_obj_t* addLbl = lv_label_create(addBtn);
                lv_label_set_text_fmt(addLbl, "Add %s", dev->roomName.c_str());
                lv_obj_set_style_text_color(addLbl, COL_TEXT, 0);
                lv_obj_set_style_text_font(addLbl, &lv_font_montserrat_16, 0);
                
                int addLbl_x = SCALE(60);
                lv_obj_align(addLbl, LV_ALIGN_LEFT_MID, addLbl_x, 0);

                lv_obj_add_event_cb(addBtn, [](lv_event_t* e) {
                    int idx = (int)(intptr_t)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
                    if (selected_group_coordinator >= 0) {
                        sonos.joinGroup(idx, selected_group_coordinator);
                        lv_label_set_text(lbl_groups_status, "Adding to group...");
                        lv_timer_handler();
                        vTaskDelay(pdMS_TO_TICKS(500));
                        sonos.updateGroupInfo();
                        refreshGroupsList();
                    }
                }, LV_EVENT_CLICKED, NULL);
            }
        }
    }
}

void createGroupsScreen() {
    scr_groups = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_groups, lv_color_hex(0x121212), 0);
    lv_obj_set_size(scr_groups, SCREEN_WIDTH_TARGET, SCREEN_HEIGHT_TARGET);

    // Create sidebar and get content area (Groups is index 1)
    lv_obj_t* content = createSettingsSidebar(scr_groups, 1);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    
    int content_width = SCALE(720);
    lv_obj_set_size(content, content_width, SCREEN_HEIGHT_TARGET);

    // Title + Refresh button row
    lv_obj_t* title_row = lv_obj_create(content);
    int title_row_width = SCALE(720);
    int title_row_height = SCALE(40);
    lv_obj_set_size(title_row, title_row_width, title_row_height);
    lv_obj_set_pos(title_row, 0, 0);
    lv_obj_set_style_bg_opa(title_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(title_row, 0, 0);
    lv_obj_set_style_pad_all(title_row, 0, 0);
    lv_obj_clear_flag(title_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl_title = lv_label_create(title_row);
    lv_label_set_text(lbl_title, "Groups");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 0, 0);

    btn_groups_scan = lv_btn_create(title_row);
    int btn_width = SCALE(110);
    int btn_height = SCALE(40);
    lv_obj_set_size(btn_groups_scan, btn_width, btn_height);
    lv_obj_align(btn_groups_scan, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_groups_scan, COL_ACCENT, 0);
    
    int btn_radius = SCALE(20);
    lv_obj_set_style_radius(btn_groups_scan, btn_radius, 0);
    lv_obj_set_style_shadow_width(btn_groups_scan, 0, 0);
    
    lv_obj_add_event_cb(btn_groups_scan, [](lv_event_t* e) {
        // Disable button during scan
        lv_obj_add_state(btn_groups_scan, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(btn_groups_scan, lv_color_hex(0x555555), LV_STATE_DISABLED);

        // Show spinner
        if (spinner_groups_scan) {
            lv_obj_remove_flag(spinner_groups_scan, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(spinner_groups_scan);
        }

        // If no speakers discovered yet, run speaker discovery first
        if (sonos.getDeviceCount() == 0) {
            lv_label_set_text(lbl_groups_status, LV_SYMBOL_REFRESH " Discovering speakers...");
            lv_refr_now(NULL);  // Force immediate refresh to show spinner
            sonos.discoverDevices();
        }

        // Now update group info
        lv_label_set_text(lbl_groups_status, LV_SYMBOL_REFRESH " Updating groups...");
        lv_refr_now(NULL);  // Force immediate refresh

        // Update group info with UI updates
        int cnt = sonos.getDeviceCount();
        for (int i = 0; i < cnt; i++) {
            lv_tick_inc(10);
            lv_timer_handler();
            lv_refr_now(NULL);
        }
        sonos.updateGroupInfo();
        refreshGroupsList();

        // Hide spinner and re-enable button
        if (spinner_groups_scan) {
            lv_obj_add_flag(spinner_groups_scan, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_clear_state(btn_groups_scan, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(btn_groups_scan, COL_ACCENT, 0);
    }, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* lbl_scan = lv_label_create(btn_groups_scan);
    lv_label_set_text(lbl_scan, LV_SYMBOL_REFRESH " Scan");
    lv_obj_set_style_text_color(lbl_scan, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(lbl_scan, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_scan);

    // Status label
    lbl_groups_status = lv_label_create(content);
    int status_y = SCALE(50);
    lv_obj_set_pos(lbl_groups_status, 0, status_y);
    lv_label_set_text(lbl_groups_status, "Tap a group to manage it");
    lv_obj_set_style_text_color(lbl_groups_status, COL_TEXT2, 0);
    lv_obj_set_style_text_font(lbl_groups_status, &lv_font_montserrat_12, 0);

    // Groups list
    list_groups = lv_obj_create(content);
    int list_width = SCALE(720);
    int list_height = SCALE(380);
    int list_y = SCALE(75);
    lv_obj_set_size(list_groups, list_width, list_height);
    lv_obj_set_pos(list_groups, 0, list_y);
    lv_obj_set_style_bg_color(list_groups, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_border_width(list_groups, 0, 0);
    lv_obj_set_style_radius(list_groups, 0, 0);
    lv_obj_set_style_pad_all(list_groups, 0, 0);
    
    int row_pad = SCALE(6);
    lv_obj_set_style_pad_row(list_groups, row_pad, 0);
    lv_obj_set_flex_flow(list_groups, LV_FLEX_FLOW_COLUMN);

    // Scrollbar styling
    int scroll_pad = SCALE(8);
    lv_obj_set_style_pad_right(list_groups, scroll_pad, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list_groups, LV_OPA_30, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(list_groups, COL_TEXT2, LV_PART_SCROLLBAR);
    
    int scroll_width = SCALE(6);
    lv_obj_set_style_width(list_groups, scroll_width, LV_PART_SCROLLBAR);
    
    int scroll_radius = SCALE(3);
    lv_obj_set_style_radius(list_groups, scroll_radius, LV_PART_SCROLLBAR);

    // Spinner for scan feedback (centered in content area, hidden by default)
    spinner_groups_scan = lv_spinner_create(content);
    int spinner_size = SCALE(100);
    lv_obj_set_size(spinner_groups_scan, spinner_size, spinner_size);
    lv_obj_center(spinner_groups_scan);
    lv_obj_set_style_arc_color(spinner_groups_scan, COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner_groups_scan, lv_color_hex(0x555555), LV_PART_MAIN);
    
    int arc_width = SCALE(10);
    lv_obj_set_style_arc_width(spinner_groups_scan, arc_width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner_groups_scan, arc_width, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(spinner_groups_scan, true, LV_PART_INDICATOR);
    lv_obj_move_foreground(spinner_groups_scan);
    lv_obj_add_flag(spinner_groups_scan, LV_OBJ_FLAG_HIDDEN);
}