/**
 * Quick Play Screen
 * 3×3 grid of favourite albums/playlists with embedded art.
 * Tapping a tile invokes the same play flow as an NFC tag tap,
 * then returns to the main screen.
 */

#include "ui_common.h"
#include "favourites_data.h"
#include "node_sonos_server.h"

// Decoded tile buffers — allocated in PSRAM on first visit, reused on subsequent opens
static uint16_t*    s_tile_buf[NUM_FAVOURITES] = {};
static lv_img_dsc_t s_tile_dsc[NUM_FAVOURITES] = {};
static bool         s_tiles_decoded = false;
static int          s_decoded_tile_size = 0;

static void decodeTiles(int tile_size) {
    if (s_tiles_decoded && s_decoded_tile_size == tile_size) return;
    for (int i = 0; i < NUM_FAVOURITES; i++) {
        if (s_tile_buf[i]) { heap_caps_free(s_tile_buf[i]); s_tile_buf[i] = nullptr; }
        if (fav_art_sizes[i] <= 4) continue;
        bool ok = decodeTileJpeg(fav_art[i], fav_art_sizes[i], tile_size, tile_size,
                                 &s_tile_dsc[i], &s_tile_buf[i]);
        if (!ok) Serial.printf("[QPLAY] tile %d decode failed\n", i);
    }
    s_tiles_decoded = true;
    s_decoded_tile_size = tile_size;
}

void createQuickPlayScreen() {
    if (scr_quickplay) {
        lv_obj_del(scr_quickplay);
        scr_quickplay = nullptr;
    }

    scr_quickplay = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_quickplay, lv_color_hex(0x121212), 0);
    lv_obj_set_size(scr_quickplay, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // Sidebar — Favourites is index 3
    lv_obj_t* content = createSettingsSidebar(scr_quickplay, 3);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    int content_width = SCALE(620);
    lv_obj_set_size(content, content_width, DISPLAY_HEIGHT);

    // Title
    lv_obj_t* lbl_title = lv_label_create(content);
    lv_label_set_text(lbl_title, "Quick Play");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
    lv_obj_set_pos(lbl_title, 0, 0);

    // Grid layout — derived from inner content area (after CONTENT_PAD padding on all sides)
    int content_pad  = SCALE(CONTENT_PAD);
    int inner_h      = DISPLAY_HEIGHT - 2 * content_pad;    // usable height inside content padding
    int inner_w      = content_width  - 2 * content_pad;    // usable width inside content padding
    int title_h      = SCALE(44);                            // space reserved for the title row
    int gap          = SCALE(10);                            // gap between tiles
    const int COLS   = 4;
    const int ROWS   = 3;

    // Tile size: fit COLS×ROWS square tiles — width-constrained on landscape (4 cols fills the space)
    int tile_h = (inner_h - title_h - (ROWS - 1) * gap) / ROWS;
    int tile_w = (inner_w            - (COLS - 1) * gap) / COLS;
    int tile_size = (tile_h < tile_w) ? tile_h : tile_w;

    int grid_w = COLS * tile_size + (COLS - 1) * gap;
    int grid_h = ROWS * tile_size + (ROWS - 1) * gap;

    // Centre grid horizontally and vertically in the space below the title
    int grid_x = (inner_w - grid_w) / 2;
    int grid_y = title_h + (inner_h - title_h - grid_h) / 2;

    // Decode tiles on first visit (lazy, one-time PSRAM allocation)
    decodeTiles(tile_size);

    lv_obj_t* grid = lv_obj_create(content);
    lv_obj_set_pos(grid, grid_x, grid_y);
    lv_obj_set_size(grid, grid_w, grid_h);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < NUM_FAVOURITES; i++) {
        int col = i % COLS;
        int row = i / COLS;
        int tx  = col * (tile_size + gap);
        int ty  = row * (tile_size + gap);

        lv_obj_t* tile = lv_btn_create(grid);
        lv_obj_set_pos(tile, tx, ty);
        lv_obj_set_size(tile, tile_size, tile_size);
        lv_obj_set_style_radius(tile, SCALE(10), 0);
        lv_obj_set_style_shadow_width(tile, 0, 0);
        lv_obj_set_style_pad_all(tile, 0, 0);
        lv_obj_set_style_bg_color(tile, COL_CARD, 0);
        lv_obj_set_style_bg_color(tile, COL_BTN_PRESSED, LV_STATE_PRESSED);
        lv_obj_set_style_clip_corner(tile, true, 0);

        if (s_tile_buf[i]) {
            lv_obj_t* img = lv_img_create(tile);
            lv_img_set_src(img, &s_tile_dsc[i]);
            lv_obj_set_size(img, tile_size, tile_size);
            lv_obj_center(img);
        } else {
            lv_obj_t* ico = lv_label_create(tile);
            lv_label_set_text(ico, LV_SYMBOL_AUDIO);
            lv_obj_set_style_text_color(ico, COL_TEXT2, 0);
            lv_obj_set_style_text_font(ico, &lv_font_montserrat_24, 0);
            lv_obj_center(ico);
        }

        lv_obj_set_user_data(tile, (void*)(uintptr_t)i);

        lv_obj_add_event_cb(tile, [](lv_event_t* e) {
            lv_obj_t* t = (lv_obj_t*)lv_event_get_target(e);
            int idx = (int)(uintptr_t)lv_obj_get_user_data(t);
            if (idx < 0 || idx >= NUM_FAVOURITES) return;

            const char* uri = fav_uris[idx];
            if (!uri || uri[0] == '\0') return;

            String room = "";
            SonosDevice* dev = sonos.getCurrentDevice();
            if (dev) room = dev->roomName;

            sonosHttpServer.sendPlayRequest(String(uri), room);
            lv_screen_load(scr_main);
        }, LV_EVENT_CLICKED, NULL);
    }
}
