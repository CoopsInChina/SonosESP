#include "touch_driver.h"
#include "ui_common.h"
#include <Wire.h>
#include <TAMC_GT911.h>
#include "config.h"

// Touch controller instance
TAMC_GT911 ts = TAMC_GT911(TOUCH_GT911_SDA, TOUCH_GT911_SCL, 
                           TOUCH_GT911_INT, TOUCH_GT911_RST,
                           TOUCH_PANEL_WIDTH, TOUCH_PANEL_HEIGHT);

static lv_indev_t *indev = nullptr;

bool touch_init(void) {
    Serial.printf("[Touch] Initializing for %d\" screen...\n", SCREEN_SIZE);
    
    Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
    ts.begin();
    
    // Set rotation based on screen
    #if SCREEN_SIZE == 7
        // 7" screen rotation
        ts.setRotation(ROTATION_INVERTED);
        Serial.println("[Touch] Using 7\" rotation (INVERTED)");
    #elif SCREEN_SIZE == 4
        // 4" screen rotation  
        ts.setRotation(ROTATION_INVERTED);
        Serial.println("[Touch] Using 4\" rotation (INVERTED)");
    #endif
    
    // LVGL input device
    indev = lv_indev_create();
    if (!indev) {
        Serial.println("[Touch] ERROR: Failed to create LVGL input device");
        return false;
    }
    
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read);
    
    Serial.println("[Touch] Touch initialized successfully!");
    return true;
}

void touch_read(lv_indev_t *indev_drv, lv_indev_data_t *data) {
    static bool was_touched = false;
    
    ts.read();
    
    if (ts.isTouched && ts.touches > 0) {
        int16_t raw_x = ts.points[0].x;
        int16_t raw_y = ts.points[0].y;
        
        #if SCREEN_SIZE == 7
            // ===== 7" SCREEN TOUCH PROCESSING =====
            // 7" doesn't need rotation (1024x600 native)
            data->point.x = map(raw_x, 0, TOUCH_PANEL_WIDTH - 1, 0, DISPLAY_WIDTH - 1);
            data->point.y = map(raw_y, 0, TOUCH_PANEL_HEIGHT - 1, 0, DISPLAY_HEIGHT - 1);
            
        #elif SCREEN_SIZE == 4
            // ===== 4" SCREEN TOUCH PROCESSING =====
            // 4" needs 90° rotation (480x800 → 800x480)
            // Apply 90° clockwise rotation
            int16_t landscape_x = raw_y;           // 0-799
            int16_t landscape_y = 479 - raw_x;     // 479-0
            
            // Clamp
            landscape_x = constrain(landscape_x, 0, 799);
            landscape_y = constrain(landscape_y, 0, 479);
            
            data->point.x = landscape_x;
            data->point.y = landscape_y;
        #endif
        
        data->state = LV_INDEV_STATE_PRESSED;
        
        if (!was_touched) {
            resetScreenTimeout();
            was_touched = true;
            
            Serial.printf("[Touch] %d\": X=%d, Y=%d\n", 
                         SCREEN_SIZE, data->point.x, data->point.y);
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        was_touched = false;
    }
}