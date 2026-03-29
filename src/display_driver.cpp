#include "display_driver.h"
#include "config.h"
#include <esp_heap_caps.h>
#include <esp_lcd_panel_ops.h>
#include <esp_private/esp_cache_private.h>
#include <driver/ppa.h>

#if SCREEN_SIZE == 7
    #include "jd9165_lcd.h"
    typedef jd9165_lcd DisplayType;
#elif SCREEN_SIZE == 4
    #include "st7701_lcd.h"
    typedef st7701_lcd DisplayType;
#endif



// ===== Global Variables =====

    static DisplayType* lcd = nullptr;
    static lv_color_t *buf1 = nullptr;
    static lv_color_t *buf2 = nullptr;
    static lv_color_t *rotate_buf = nullptr;  // Rotation buffer
    static lv_display_t *disp = nullptr;
    static bsp_lcd_handles_t lcd_handles;

    
// Software rotation function - rotate landscape 800x480 to portrait 480x800
static void rotate_image_90(const uint16_t *src, uint16_t *dst, int width, int height) {
    // Block sizes for cache-efficient rotation
    constexpr int block_w = 256;
    constexpr int block_h = 32;

    for (int i = 0; i < height; i += block_h) {
        int max_height = (i + block_h > height) ? height : (i + block_h);

        for (int j = 0; j < width; j += block_w) {
            int max_width = (j + block_w > width) ? width : (j + block_w);

            for (int x = i; x < max_height; x++) {
                for (int y = j; y < max_width; y++) {
                    // Source pixel at (x, y) -> reading as (row, col)
                    const uint16_t *src_pixel = src + (x * width + y);

                    // 90° rotation formula from reference: (x, y) -> (y, height - 1 - x)
                    uint16_t *dst_pixel = dst + (y * height + (height - 1 - x));
                    *dst_pixel = *src_pixel;
                }
            }
        }
    }
}
    // ===== Display Initialization ===
    bool display_init(void) {
        Serial.printf("[Display] Initializing MIPI DSI interface for %s...\n", DISPLAY_MODEL);

        // Create JD9165 LCD instance
        lcd = new DisplayType(LCD_RST);

        if (!lcd) {
            Serial.println("[Display] ERROR: Failed to create LCD instance!");
            return false;
        }

        // Initialize the LCD
        lcd->begin();
        lcd->get_handle(&lcd_handles);

        Serial.printf("[Display] %s LCD initialized successfully\n", DISPLAY_MODEL);

        

    // Common: Allocate LVGL buffers
    size_t lvgl_size = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(lv_color_t);
    buf1 = (lv_color_t *)heap_caps_malloc(lvgl_size, MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t *)heap_caps_malloc(lvgl_size, MALLOC_CAP_SPIRAM);
    
    // Screen-specific: Rotation buffer
    #if SCREEN_SIZE == 4
        // 4" ST7701 needs rotation buffer
        size_t rotate_size = DISPLAY_HEIGHT * DISPLAY_WIDTH * sizeof(lv_color_t);
        rotate_buf = (lv_color_t *)heap_caps_malloc(rotate_size, MALLOC_CAP_SPIRAM);
    #elif SCREEN_SIZE == 7
        // 7" JD9165 doesn't need rotation
        rotate_buf = nullptr;
    #endif
    
    // Validate
    bool success = (buf1 != nullptr) && (buf2 != nullptr);
    
    #if SCREEN_SIZE == 4
        success = success && (rotate_buf != nullptr);
    #endif
    
    if (!success) {
        Serial.println("[Display] ERROR: Buffer allocation failed!");
        if (buf1) heap_caps_free(buf1);
        if (buf2) heap_caps_free(buf2);
        if (rotate_buf) heap_caps_free(rotate_buf);
        return false;
    }
    
    // Log
    Serial.printf("[Display] LVGL buffers: %zu bytes each (%dx%d)\n",
                  lvgl_size, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    #if SCREEN_SIZE == 4
        Serial.printf("[Display] Rotate buffer: %zu bytes (%dx%d)\n",
                      rotate_size, DISPLAY_HEIGHT, DISPLAY_WIDTH);
    #endif
    
    Serial.printf("[Display] Free PSRAM: %zu bytes\n",
                  heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
     // Create display with correct dimensions
    disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!disp) {
        Serial.println("[Display] ERROR: Failed to create LVGL display");
        return false;
    }
    
    // Set flush callback
    lv_display_set_flush_cb(disp, display_flush);
    
    // Set buffers
    size_t buffer_size = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(lv_color_t);
    lv_display_set_buffers(disp, buf1, buf2, buffer_size, LV_DISPLAY_RENDER_MODE_FULL);
    
    // Configure rotation
    #if SCREEN_SIZE == 4
        // 4" ST7701: Manual rotation in flush callback
        // DON'T use lv_display_set_rotation - we rotate manually
        Serial.println("[Display] Ready! 800x480 landscape with manual 90° rotation to portrait panel");
    #elif SCREEN_SIZE == 7
        // 7" JD9165: No rotation needed
        Serial.println("[Display] Ready for 1024x600 display (no rotation, full-width flush)");
    #endif

    return true;
    }

   // ===== 7" JD9165 Flush (No Rotation) =====
#if SCREEN_SIZE == 7
void display_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map) {
    // 1. Essential NULL checks
    if (!lcd || !lcd_handles.panel || !buf1) {
        lv_display_flush_ready(disp_drv);
        return;
    }

    // 2. Calculate dimensions of the area to update
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);

    // 3. Copy LVGL's rendered data into the main framebuffer (buf1)
    lv_color_t *dest = buf1 + area->y1 * DISPLAY_WIDTH + area->x1;
    for (int y = 0; y < h; y++) {
        memcpy(&dest[y * DISPLAY_WIDTH],
            &((lv_color_t *)px_map)[y * w],
            w * sizeof(lv_color_t));
    }

    // 4. Send the updated region to the physical screen
    lcd->lcd_draw_bitmap(area->x1, area->y1,
                        area->x2 + 1, area->y2 + 1,
                        (uint16_t *)buf1);

    // 5. Notify LVGL that the flush is complete
    lv_display_flush_ready(disp_drv);
}
#endif

// ===== 4" ST7701 Flush (With 90° Rotation) =====
#if SCREEN_SIZE == 4
void display_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map) {
    if (!lcd || !lcd_handles.panel || !rotate_buf) {
        lv_display_flush_ready(disp_drv);
        return;
    }

    // Rotate the entire frame from landscape 800x480 to portrait 480x800 for panel
// Software rotation only
    rotate_image_90((uint16_t *)px_map, (uint16_t *)rotate_buf, DISPLAY_WIDTH, DISPLAY_HEIGHT);



// Send rotated buffer to panel in portrait orientation
    lcd->lcd_draw_bitmap(0, 0, PANEL_WIDTH, PANEL_HEIGHT, (uint16_t *)rotate_buf);

    lv_display_flush_ready(disp_drv);

}
#endif

    // ===== Display Brightness Control =====
    void display_set_brightness(uint8_t brightness_percent) {
        if (lcd) {
            if (brightness_percent > 100) brightness_percent = 100;
            lcd->example_bsp_set_lcd_backlight(brightness_percent);
        }
    }


// ===== Display Deinitialization =====
void display_deinit() {
    // LCD instance
    if (lcd) {
        delete lcd;
        lcd = nullptr;
    }
    
    // Common LVGL buffers
    if (buf1) {
        heap_caps_free(buf1);
        buf1 = nullptr;
    }
    
    if (buf2) {
        heap_caps_free(buf2);
        buf2 = nullptr;
    }
    
    // 4" specific: Rotation buffer
    #if SCREEN_SIZE == 4
        if (rotate_buf) {
            heap_caps_free(rotate_buf);
            rotate_buf = nullptr;
        }
    #endif

    
    // Reset handles
    memset(&lcd_handles, 0, sizeof(lcd_handles));
    disp = nullptr;
    }
