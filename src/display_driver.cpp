#include "display_driver.h"
#include "jd9165_lcd.h"
#include <esp_heap_caps.h>
#include <esp_lcd_panel_ops.h>
#include <driver/ppa.h>

// ===== Display Configuration =====
#define ROTATION_DEGREE 0             // No rotation
#define USE_PPA_ACCELERATION 0        // PPA not used
#define DISPLAY_WIDTH  1024           // Panel resolution: 1024x600
#define DISPLAY_HEIGHT 600

// ===== Global Variables =====
static jd9165_lcd* lcd = nullptr;
static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;
static lv_display_t *disp = nullptr;
static bsp_lcd_handles_t lcd_handles;

// ===== Display Initialization =====
bool display_init(void) {
    Serial.println("[Display] Initializing MIPI DSI interface for jC1060P470C...");

    // Create JD9165 LCD instance
    lcd = new jd9165_lcd(LCD_RST);

    if (!lcd) {
        Serial.println("[Display] ERROR: Failed to create LCD instance!");
        return false;
    }

    // Initialize the LCD
    lcd->begin();
    lcd->get_handle(&lcd_handles);

    Serial.println("[Display] JD9165 LCD initialized successfully");

    

    // Allocate LVGL buffers in PSRAM (1024x600, 16bpp = 2 bytes/pixel)
    buf1 = (lv_color_t *)heap_caps_malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t *)heap_caps_malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (!buf1 || !buf2) {
        Serial.println("[Display] ERROR: Failed to allocate LVGL buffers!");
        if (buf1) heap_caps_free(buf1);
        if (buf2) heap_caps_free(buf2);
        return false;
    }

    Serial.printf("[Display] LVGL buffers: %zu bytes each (1024x600 @ 16bpp)\n",
                  DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(lv_color_t));
    Serial.printf("[Display] Free PSRAM: %zu bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    // Create the LVGL display with correct resolution
    disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!disp) {
        Serial.println("[Display] ERROR: Failed to create LVGL display");
        return false;
    }

    // Set flush callback and double buffering
    lv_display_set_flush_cb(disp, display_flush);
    lv_display_set_buffers(disp, buf1, buf2, DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_FULL);

    Serial.println("[Display] Ready for 1024x600 display (no rotation, full-width flush)");
    return true;
}

// ===== Display Flush (Fixes 45° Skew by Drawing Full 1024px Width) =====
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
    // Source line: cast px_map to lv_color_t* and index with [y * w]
    // Destination line: use the correct full-screen stride (DISPLAY_WIDTH)
    memcpy(&dest[y * DISPLAY_WIDTH],
           &((lv_color_t *)px_map)[y * w],
           w * sizeof(lv_color_t));
}

// 4. Send the updated region to the physical screen
// Using the region from LVGL (area) and the now-correct data in buf1
lcd->lcd_draw_bitmap(area->x1, area->y1,
                     area->x2 + 1, area->y2 + 1,
                     (uint16_t *)buf1);

// 5. CRITICAL: Notify LVGL that the flush is complete
lv_display_flush_ready(disp_drv);


}

// ===== Display Brightness Control =====
void display_set_brightness(uint8_t brightness_percent) {
    if (lcd) {
        if (brightness_percent > 100) brightness_percent = 100;
        lcd->example_bsp_set_lcd_backlight(brightness_percent);
    }
}

// ===== Display Deinitialization =====
void display_deinit() {
    if (lcd) {
        delete lcd;
        lcd = nullptr;
    }
    if (buf1) {
        heap_caps_free(buf1);
        buf1 = nullptr;
    }
    if (buf2) {
        heap_caps_free(buf2);
        buf2 = nullptr;
    }
}