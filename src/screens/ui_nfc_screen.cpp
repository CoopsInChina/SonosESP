#include "ui_common.h"

#if SCREEN_SIZE == 7

#include "pn532_nfc_manager.h"
#include "node_sonos_server.h"
#include <Arduino.h>

extern PN532_NFCManager nfcManager;

lv_obj_t* scr_nfc = nullptr;

// ---------------------------------------------------------------------------
// Static widget pointers — used by lambda callbacks (no-capture)
// ---------------------------------------------------------------------------
static lv_obj_t* s_lbl_status  = nullptr;
static lv_obj_t* s_toggle      = nullptr;
static lv_obj_t* s_lbl_server  = nullptr;

static void updateStatusLabel() {
    if (!s_lbl_status) return;
    if (nfcManager.isConnected()) {
        String txt = "Connected — " + nfcManager.getFirmwareVersionStr();
        lv_label_set_text(s_lbl_status, txt.c_str());
        lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(0x1DB954), 0);
    } else {
        lv_label_set_text(s_lbl_status, "Not connected");
        lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(0xFF5555), 0);
    }
}

static void updateServerLabel() {
    if (!s_lbl_server) return;
    switch (sonosHttpServer.getState()) {
        case NodeSonosServer::State::FOUND:
            lv_label_set_text(s_lbl_server, sonosHttpServer.getBaseUrl().c_str());
            lv_obj_set_style_text_color(s_lbl_server, lv_color_hex(0x1DB954), 0);
            break;
        case NodeSonosServer::State::SCANNING:
            lv_label_set_text(s_lbl_server, "Scanning network...");
            lv_obj_set_style_text_color(s_lbl_server, lv_color_hex(0xFFAA00), 0);
            break;
        case NodeSonosServer::State::NOT_FOUND:
            lv_label_set_text(s_lbl_server, "Not found");
            lv_obj_set_style_text_color(s_lbl_server, lv_color_hex(0xFF5555), 0);
            break;
        default:
            lv_label_set_text(s_lbl_server, "—");
            lv_obj_set_style_text_color(s_lbl_server, lv_color_hex(0xAAAAAA), 0);
            break;
    }
}

void createNFCSettingsScreen() {
    scr_nfc = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_nfc, lv_color_hex(0x121212), 0);
    lv_obj_set_size(scr_nfc, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // Register status callback so background scan updates the UI
    sonosHttpServer.setStatusCallback(updateServerLabel);

    // Sidebar
    lv_obj_t* content = createSettingsSidebar(scr_nfc, 7);
    lv_obj_set_size(content, SCALE(620), DISPLAY_HEIGHT);

    // Title
    lv_obj_t* lbl_title = lv_label_create(content);
    lv_label_set_text(lbl_title, "NFC");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(lbl_title, 0, SCALE(20));

    // ── NFC reader status row ────────────────────────────────────────────────
    lv_obj_t* row_status = lv_obj_create(content);
    lv_obj_set_size(row_status, SCALE(550), SCALE(40));
    lv_obj_set_pos(row_status, 0, SCALE(70));
    lv_obj_set_style_bg_opa(row_status, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_status, 0, 0);
    lv_obj_set_style_pad_all(row_status, 0, 0);

    lv_obj_t* lbl_status_hdr = lv_label_create(row_status);
    lv_label_set_text(lbl_status_hdr, "NFC Reader:");
    lv_obj_set_style_text_font(lbl_status_hdr, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_status_hdr, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(lbl_status_hdr, LV_ALIGN_LEFT_MID, 0, 0);

    s_lbl_status = lv_label_create(row_status);
    lv_obj_set_style_text_font(s_lbl_status, &lv_font_montserrat_18, 0);
    lv_obj_align(s_lbl_status, LV_ALIGN_LEFT_MID, SCALE(130), 0);
    updateStatusLabel();

    // ── Enable toggle row ────────────────────────────────────────────────────
    lv_obj_t* row_toggle = lv_obj_create(content);
    lv_obj_set_size(row_toggle, SCALE(550), SCALE(40));
    lv_obj_set_pos(row_toggle, 0, SCALE(130));
    lv_obj_set_style_bg_opa(row_toggle, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_toggle, 0, 0);
    lv_obj_set_style_pad_all(row_toggle, 0, 0);

    lv_obj_t* lbl_toggle = lv_label_create(row_toggle);
    lv_label_set_text(lbl_toggle, "NFC Enabled");
    lv_obj_set_style_text_font(lbl_toggle, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_toggle, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_toggle, LV_ALIGN_LEFT_MID, 0, 0);

    s_toggle = lv_switch_create(row_toggle);
    lv_obj_align(s_toggle, LV_ALIGN_RIGHT_MID, 0, 0);
    if (nfcManager.isScanning()) lv_obj_add_state(s_toggle, LV_STATE_CHECKED);

    lv_obj_add_event_cb(s_toggle, [](lv_event_t* e) {
        lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
        bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
        if (on) {
            nfcManager.reconnect();
            nfcManager.setScanning(true);
        } else {
            nfcManager.setScanning(false);
        }
        updateStatusLabel();
        Serial.printf("[NFC] Scanning %s\n", on ? "enabled" : "disabled");
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // ── HTTP server section ───────────────────────────────────────────────────
    lv_obj_t* lbl_srv_hdr = lv_label_create(content);
    lv_label_set_text(lbl_srv_hdr, "HTTP Server");
    lv_obj_set_style_text_font(lbl_srv_hdr, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_srv_hdr, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_pos(lbl_srv_hdr, 0, SCALE(200));

    s_lbl_server = lv_label_create(content);
    lv_obj_set_style_text_font(s_lbl_server, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(s_lbl_server, 0, SCALE(238));

    updateServerLabel();
}

#endif // SCREEN_SIZE == 7
