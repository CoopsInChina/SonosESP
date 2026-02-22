/**
 * Clock / Screensaver Screen
 *
 * Full-screen clock with NTP time, optional loremflickr.com photo background.
 * Activates based on user-configured inactivity or playback-state triggers.
 * All Sonos art/lyrics tasks are stopped while the clock is shown.
 *
 * Architecture:
 *  - checkClockTrigger()  : called every loop() iteration, drives state machine
 *  - clockBgTask          : FreeRTOS task that downloads + decodes Flickr JPEG
 *  - exitClockScreen()    : called from touch handler or when music resumes
 */

#include "ui_common.h"
#include "config.h"
#include "clock_screen.h"

LV_FONT_DECLARE(lv_font_montserrat_140);
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// Undefine shared macros before JPEGDEC
#undef INTELSHORT
#undef INTELLONG
#undef MOTOSHORT
#undef MOTOLONG
#include <JPEGDEC.h>

// ============================================================================
// JPEGDEC callback globals for clock background (file-scoped, not shared)
// ============================================================================
static uint16_t* clk_jpeg_dest   = nullptr;  // Points to clock_bg_buffer during decode
static int       clk_jpeg_stride = 0;        // Full image width (pixels) for row offset
static int       clock_decoded_w = 0;        // Actual pixels-written width  (set by callback, NOT getWidth())
static int       clock_decoded_h = 0;        // Actual pixels-written height

static int IRAM_ATTR clockJpegCallback(JPEGDRAW* pDraw) {
    if (!clk_jpeg_dest) return 0;

    for (int y = 0; y < pDraw->iHeight; y++) {
        int dst_y = pDraw->y + y;
        if (dst_y < 0 || dst_y >= CLOCK_BG_HEIGHT) continue;
        int dst_x = pDraw->x;
        if (dst_x < 0 || dst_x >= CLOCK_BG_WIDTH) continue;
        int copy_w = min(pDraw->iWidth, CLOCK_BG_WIDTH - dst_x);
        memcpy(
            clk_jpeg_dest + dst_y * clk_jpeg_stride + dst_x,
            pDraw->pPixels + y * pDraw->iWidth,
            (size_t)copy_w * 2
        );
        // Track the rightmost and bottommost pixel actually written.
        // This captures the true decoded region regardless of what getWidth()/getHeight() report,
        // which matters if the JPEG is partially decoded (e.g. truncated download or unsupported
        // encoding). LV_IMAGE_ALIGN_STRETCH will then scale this real region to fill the widget.
        int right  = dst_x + copy_w;
        int bottom = dst_y + 1;
        if (right  > clock_decoded_w) clock_decoded_w = right;
        if (bottom > clock_decoded_h) clock_decoded_h = bottom;
    }
    return 1;  // Always continue — never abort decode mid-image
}

// ============================================================================
// Clock tick — updates time and date labels (lv_timer, main thread only)
// ============================================================================
static lv_timer_t* clock_tick_timer = nullptr;

static void clock_tick_cb(lv_timer_t* /*timer*/) {
    if (!clock_time_lbl || !clock_date_lbl) return;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 100)) {
        // NTP not synced yet — show dashes
        lv_label_set_text(clock_time_lbl, "--:--");
        lv_label_set_text(clock_date_lbl, "Waiting for NTP...");
        return;
    }

    char time_str[8];
    if (clock_12h) {
        strftime(time_str, sizeof(time_str), "%I:%M", &timeinfo);
        // Strip leading zero: "09:30" → "9:30"
        const char* t = (time_str[0] == '0') ? time_str + 1 : time_str;
        lv_label_set_text(clock_time_lbl, t);
    } else {
        strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
        lv_label_set_text(clock_time_lbl, time_str);
    }

    char date_str[32];
    strftime(date_str, sizeof(date_str), "%a, %b %d", &timeinfo);
    lv_label_set_text(clock_date_lbl, date_str);
}

// ============================================================================
// Background picture download + JPEG decode task (FreeRTOS, core 0)
// ============================================================================
void clockBgTask(void* /*param*/) {
    // Give the clock screen a moment to fully render before first download
    vTaskDelay(pdMS_TO_TICKS(1500));

    while (!clock_bg_shutdown_requested) {
        // --- HTTPS cooldown (same guard as art/lyrics tasks) ---
        while (!clock_bg_shutdown_requested) {
            unsigned long since_https = millis() - last_https_end_ms;
            if (since_https >= OTA_HTTPS_COOLDOWN_MS) break;
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        if (clock_bg_shutdown_requested) break;

        // --- Acquire network mutex ---
        if (xSemaphoreTake(network_mutex, pdMS_TO_TICKS(8000)) != pdTRUE) {
            Serial.println("[CLKBG] Could not acquire network mutex, skipping");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        if (clock_bg_shutdown_requested) {
            xSemaphoreGive(network_mutex);
            break;
        }

        // --- Download loremflickr.com JPEG ---
        uint8_t* dl_buf = (uint8_t*)heap_caps_malloc(
            CLOCK_BG_MAX_DL_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        int dl_total = 0;

        if (dl_buf) {
            WiFiClientSecure secure_client;
            secure_client.setInsecure();  // No cert verification for photo CDN
            HTTPClient http;
            http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // loremflickr → Flickr CDN cross-domain
            http.setTimeout(15000);

            // loremflickr.com redirects to Flickr CDN photos (baseline JPEG — required, picsum serves
            // progressive JPEG which JPEGDEC can only partially decode at 1/8 resolution).
            // Keyword is user-selected from settings; empty = truly random (no tag filter).
            // esp_random() gives a 32-bit seed so each refresh fetches a different image.
            char url[96];
            const char* kw = CLOCK_BG_KEYWORDS[clock_bg_kw_idx].kw;
            if (kw[0] == '\0') {
                snprintf(url, sizeof(url),
                         "https://loremflickr.com/%d/%d?lock=%lu",
                         CLOCK_BG_WIDTH, CLOCK_BG_HEIGHT, (unsigned long)esp_random());
            } else {
                snprintf(url, sizeof(url),
                         "https://loremflickr.com/%d/%d/%s?lock=%lu",
                         CLOCK_BG_WIDTH, CLOCK_BG_HEIGHT, kw, (unsigned long)esp_random());
            }

            if (http.begin(secure_client, url)) {
                int code = http.GET();
                if (code == HTTP_CODE_OK) {
                    WiFiClient* stream = http.getStreamPtr();
                    uint32_t t0 = millis();
                    while (dl_total < CLOCK_BG_MAX_DL_SIZE && !clock_bg_shutdown_requested) {
                        if (millis() - t0 > 20000) break;     // 20-second overall timeout
                        if (!stream->connected() && !stream->available()) break;  // Server closed
                        int avail = stream->available();
                        if (avail <= 0) { vTaskDelay(pdMS_TO_TICKS(5)); continue; }
                        int to_read = min(avail, CLOCK_BG_MAX_DL_SIZE - dl_total);
                        int n = stream->readBytes(dl_buf + dl_total, to_read);
                        if (n > 0) dl_total += n;
                        vTaskDelay(pdMS_TO_TICKS(5));
                    }
                    Serial.printf("[CLKBG] Downloaded %d bytes\n", dl_total);
                } else {
                    Serial.printf("[CLKBG] HTTP error: %d\n", code);
                }
                http.end();
                secure_client.stop();
            }

            last_https_end_ms   = millis();
            last_network_end_ms = millis();
        }

        xSemaphoreGive(network_mutex);

        // --- Decode JPEG into clock_bg_buffer ---
        if (dl_total > 0 && clock_bg_buffer && !clock_bg_shutdown_requested) {
            Serial.printf("[CLKBG] Decoding %d bytes JPEG\n", dl_total);

            // Allocate JPEGDEC in PSRAM (~18KB struct — too large for task stack)
            JPEGDEC* jpeg = (JPEGDEC*)heap_caps_malloc(sizeof(JPEGDEC), MALLOC_CAP_SPIRAM);
            if (!jpeg) {
                Serial.println("[CLKBG] Failed to alloc JPEGDEC");
            } else {
                memset(jpeg, 0, sizeof(JPEGDEC));  // Zero-init (JPEGDEC is POD; equivalent to default construction)
                clk_jpeg_dest = clock_bg_buffer;

                if (jpeg->openRAM(dl_buf, dl_total, clockJpegCallback)) {
                    Serial.printf("[CLKBG] JPEG declared: %dx%d (expected %dx%d)\n",
                                  jpeg->getWidth(), jpeg->getHeight(),
                                  CLOCK_BG_WIDTH, CLOCK_BG_HEIGHT);
                    clk_jpeg_stride = CLOCK_BG_WIDTH;  // dest buffer stride (fixed, callback clips overflow)
                    clock_decoded_w = 0;               // reset — callback accumulates actual written region
                    clock_decoded_h = 0;
                    memset(clock_bg_buffer, 0, (size_t)CLOCK_BG_WIDTH * CLOCK_BG_HEIGHT * 2);
                    jpeg->setPixelType(RGB565_LITTLE_ENDIAN);  // Match LVGL color format
                    int rc = jpeg->decode(0, 0, 0);            // 0 = full resolution
                    jpeg->close();
                    Serial.printf("[CLKBG] Decode rc=%d, actual pixels: %dx%d\n",
                                  rc, clock_decoded_w, clock_decoded_h);
                    if (rc == 1 && clock_decoded_w > 0 && clock_decoded_h > 0) {
                        clock_bg_ready = true;
                        Serial.println("[CLKBG] Background ready");
                    } else {
                        Serial.printf("[CLKBG] JPEG decode failed or empty\n");
                    }
                } else {
                    Serial.println("[CLKBG] JPEG open failed");
                }
                clk_jpeg_dest = nullptr;

                heap_caps_free(jpeg);  // jpeg->close() already released internal buffers
            }
        }

        if (dl_buf) {
            heap_caps_free(dl_buf);
            dl_buf = nullptr;
        }

        // --- Wait refresh interval, checking shutdown flag every 500ms ---
        uint32_t wait_ms = (uint32_t)clock_refresh_min * 60000UL;
        uint32_t waited  = 0;
        while (waited < wait_ms && !clock_bg_shutdown_requested) {
            vTaskDelay(pdMS_TO_TICKS(500));
            waited += 500;
        }
    }

    Serial.println("[CLKBG] Task exiting");
    clockBgTaskHandle = nullptr;
    vTaskDelete(NULL);
}

// ============================================================================
// Create the clock screen (called once at boot, like all other screens)
// ============================================================================
void createClockScreen() {
    scr_clock = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_clock, lv_color_hex(0x000000), 0);
    lv_obj_clear_flag(scr_clock, LV_OBJ_FLAG_SCROLLABLE);

    // Background image (hidden until clockBgTask fetches a photo)
    clock_bg_img = lv_img_create(scr_clock);
    lv_obj_set_size(clock_bg_img, CLOCK_BG_WIDTH, CLOCK_BG_HEIGHT);
    lv_obj_set_pos(clock_bg_img, 0, 0);
    lv_obj_set_style_img_opa(clock_bg_img, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(clock_bg_img, LV_OBJ_FLAG_CLICKABLE);
    // Stretch image to fill widget — handles any decoded size (especially when
    // the JPEG dimensions differ from CLOCK_BG_WIDTH×CLOCK_BG_HEIGHT)
    lv_image_set_align(clock_bg_img, LV_IMAGE_ALIGN_STRETCH);

    // Semi-transparent dark overlay so text is always readable over photos
    lv_obj_t* overlay = lv_obj_create(scr_clock);
    lv_obj_set_size(overlay, CLOCK_BG_WIDTH, CLOCK_BG_HEIGHT);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, 160, 0);  // ~63% dark veil
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Time label — HH:MM in native 120px Montserrat (crisp, no scaling)
    clock_time_lbl = lv_label_create(scr_clock);
    lv_label_set_text(clock_time_lbl, "--:--");
    lv_obj_set_style_text_font(clock_time_lbl, &lv_font_montserrat_140, 0);
    lv_obj_set_style_text_color(clock_time_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(clock_time_lbl, LV_ALIGN_CENTER, 0, -30);

    // Date label — "Thu, May 16" small and dim below the time
    clock_date_lbl = lv_label_create(scr_clock);
    lv_label_set_text(clock_date_lbl, "");
    lv_obj_set_style_text_font(clock_date_lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(clock_date_lbl, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(clock_date_lbl, LV_ALIGN_CENTER, 0, 90);

    // Tap-to-dismiss hint at bottom
    lv_obj_t* hint = lv_label_create(scr_clock);
    lv_label_set_text(hint, "Tap to return");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x555555), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -16);

    // Touch anywhere on the screen to dismiss
    lv_obj_add_flag(scr_clock, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(scr_clock, [](lv_event_t* /*e*/) {
        exitClockScreen();
    }, LV_EVENT_CLICKED, NULL);
}

// ============================================================================
// exitClockScreen — called from touch handler OR when music auto-resumes.
// Starts the CLOCK_EXITING state; cleanup finishes in checkClockTrigger().
// MUST be called from the main thread (LVGL context).
// ============================================================================
void exitClockScreen() {
    if (clock_state != CLOCK_ACTIVE) return;

    Serial.println("[CLOCK] Exiting clock screen");
    clock_state = CLOCK_EXITING;
    clock_exiting_start_ms = millis();

    // Stop the 1-second tick timer immediately
    if (clock_tick_timer) {
        lv_timer_delete(clock_tick_timer);
        clock_tick_timer = nullptr;
    }

    // Tell background task to stop
    clock_bg_shutdown_requested = true;

    // Record exit time so we don't immediately re-trigger
    last_clock_exit_ms = millis();
    resetScreenTimeout();

    // Restart art + lyrics tasks — Sonos may have a pending art URL
    art_shutdown_requested    = false;
    lyrics_shutdown_requested = false;  // Was set on clock entry; must clear on exit
    last_art_url = "";  // Force re-fetch since track may have changed
    if (!albumArtTaskHandle) {
        xTaskCreatePinnedToCore(
            albumArtTask, "Art",
            ART_TASK_STACK_SIZE, NULL, ART_TASK_PRIORITY,
            &albumArtTaskHandle, 0);
    }

    // Transition back to main screen
    lv_screen_load_anim(scr_main, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
}

// ============================================================================
// checkClockTrigger — call every loop() iteration.
// Drives the clock state machine without blocking the main thread.
// ============================================================================
void checkClockTrigger() {
    switch (clock_state) {

        // ------------------------------------------------------------------
        case CLOCK_IDLE: {
            // Not re-triggering soon after a manual dismiss
            if (millis() - last_clock_exit_ms < CLOCK_EXIT_COOLDOWN_MS) return;
            if (clock_mode == CLOCK_MODE_DISABLED) return;

            uint32_t inactivity_ms = (uint32_t)clock_timeout_min * 60000UL;
            if (millis() - last_touch_time < inactivity_ms) return;

            bool trigger = false;
            switch (clock_mode) {
                case CLOCK_MODE_INACTIVITY: trigger = true;                               break;
                case CLOCK_MODE_PAUSED:     trigger = !ui_playing;                        break;
                case CLOCK_MODE_NOTHING:    trigger = !ui_playing && ui_title.isEmpty();  break;
            }
            if (!trigger) return;

            Serial.println("[CLOCK] Trigger — entering clock screen");
            clock_state            = CLOCK_ENTERING;
            clock_entering_start_ms = millis();

            // Signal art + lyrics to stop
            art_shutdown_requested    = true;
            lyrics_shutdown_requested = true;
            break;
        }

        // ------------------------------------------------------------------
        case CLOCK_ENTERING: {
            bool art_done    = (albumArtTaskHandle == nullptr);
            bool lyrics_done = (lyricsTaskHandle   == nullptr);
            bool timed_out   = (millis() - clock_entering_start_ms > CLOCK_ENTER_TIMEOUT_MS);

            // Wait until both tasks have exited, or until the timeout expires
            if ((!art_done || !lyrics_done) && !timed_out) return;

            // Apply timezone via POSIX TZ string
            setenv("TZ", CLOCK_ZONES[clock_tz_idx].posix, 1);
            tzset();

            // Allocate and zero pixel buffer for background (800×480 RGB565 = 768 KB in PSRAM)
            if (clock_picsum_enabled && !clock_bg_buffer) {
                size_t buf_sz = (size_t)CLOCK_BG_WIDTH * CLOCK_BG_HEIGHT * 2;
                clock_bg_buffer = (uint16_t*)heap_caps_malloc(
                    buf_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (!clock_bg_buffer) {
                    Serial.println("[CLOCK] Failed to allocate bg buffer — photo disabled");
                } else {
                    memset(clock_bg_buffer, 0, buf_sz);  // Zero so partial decodes show black, not garbage
                }
            }

            // Start background photo download task
            if (clock_picsum_enabled && clock_bg_buffer) {
                clock_bg_shutdown_requested = false;
                clock_bg_ready              = false;
                memset(&clock_bg_dsc, 0, sizeof(clock_bg_dsc));
                xTaskCreatePinnedToCore(
                    clockBgTask, "ClkBg",
                    CLOCK_BG_TASK_STACK, NULL, 1,
                    &clockBgTaskHandle, 0);
            }

            // Start 1-second clock tick
            clock_tick_timer = lv_timer_create(clock_tick_cb, 1000, NULL);
            clock_tick_cb(nullptr);  // Immediate first update

            clock_state = CLOCK_ACTIVE;
            lv_screen_load_anim(scr_clock, LV_SCR_LOAD_ANIM_FADE_IN, 500, 0, false);
            Serial.println("[CLOCK] Clock screen active");
            break;
        }

        // ------------------------------------------------------------------
        case CLOCK_ACTIVE: {
            // Auto-dismiss when music starts playing — but NOT for CLOCK_MODE_INACTIVITY.
            // That mode shows the clock regardless of play state (dismiss by touch only).
            // For PAUSED / NOTHING modes the clock only made sense because nothing was playing,
            // so resuming playback should close it.
            if (ui_playing && clock_mode != CLOCK_MODE_INACTIVITY) {
                Serial.println("[CLOCK] Music playing — auto-exiting clock");
                exitClockScreen();
                break;
            }

            // Apply new background image when the bg task has one ready
            if (clock_bg_ready && clock_bg_buffer && clock_bg_img) {
                clock_bg_ready = false;

                memset(&clock_bg_dsc, 0, sizeof(clock_bg_dsc));
                // Use actual decoded dimensions — LV_IMAGE_ALIGN_STRETCH will scale to widget size.
                // stride is the fixed buffer row width (800px), not the decoded image width.
                clock_bg_dsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
                clock_bg_dsc.header.cf     = LV_COLOR_FORMAT_RGB565;
                clock_bg_dsc.header.w      = (uint32_t)(clock_decoded_w > 0 ? clock_decoded_w : CLOCK_BG_WIDTH);
                clock_bg_dsc.header.h      = (uint32_t)(clock_decoded_h > 0 ? clock_decoded_h : CLOCK_BG_HEIGHT);
                clock_bg_dsc.header.stride = CLOCK_BG_WIDTH * 2;  // bytes per row in PSRAM buffer
                clock_bg_dsc.data_size     = CLOCK_BG_WIDTH * CLOCK_BG_HEIGHT * 2;
                clock_bg_dsc.data          = (const uint8_t*)clock_bg_buffer;

                lv_img_set_src(clock_bg_img, &clock_bg_dsc);
                lv_obj_set_style_img_opa(clock_bg_img, LV_OPA_TRANSP, 0);  // Start hidden
                lv_obj_invalidate(clock_bg_img);

                // Fade in the new photo over 1.5 seconds
                lv_anim_t a;
                lv_anim_init(&a);
                lv_anim_set_var(&a, clock_bg_img);
                lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
                lv_anim_set_duration(&a, 300);
                lv_anim_set_exec_cb(&a, [](void* obj, int32_t v) {
                    lv_obj_set_style_img_opa((lv_obj_t*)obj, (lv_opa_t)v, 0);
                });
                lv_anim_start(&a);
            }
            break;
        }

        // ------------------------------------------------------------------
        case CLOCK_EXITING: {
            // Wait for the background task to finish writing its buffer
            bool bg_done  = (clockBgTaskHandle == nullptr);
            bool timed_out = (millis() - clock_exiting_start_ms > 2000);

            if (!bg_done && !timed_out && clock_picsum_enabled) return;

            // Hide the background image BEFORE freeing the buffer to prevent
            // LVGL from rendering a dangling pointer during the transition.
            if (clock_bg_img) {
                lv_obj_set_style_img_opa(clock_bg_img, LV_OPA_TRANSP, 0);
                lv_img_set_src(clock_bg_img, nullptr);
            }

            // Now safe to free the pixel buffer
            if (clock_bg_buffer) {
                heap_caps_free(clock_bg_buffer);
                clock_bg_buffer = nullptr;
            }
            // Zero out descriptor so it no longer references freed memory
            memset(&clock_bg_dsc, 0, sizeof(clock_bg_dsc));

            clock_state = CLOCK_IDLE;
            Serial.println("[CLOCK] Exit complete");
            break;
        }
    }
}
