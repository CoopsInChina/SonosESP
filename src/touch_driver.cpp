#include "touch_driver.h"
#include "ui_common.h"  // This should have resetScreenTimeout() declaration
#include <Wire.h>
#include <TAMC_GT911.h>

// Touch panel dimensions (480×800 portrait)
#define TOUCH_PANEL_WIDTH  1024
#define TOUCH_PANEL_HEIGHT 600
#define DISPLAY_WIDTH      1024
#define DISPLAY_HEIGHT     600

TAMC_GT911 ts = TAMC_GT911(TOUCH_GT911_SDA, TOUCH_GT911_SCL, TOUCH_GT911_INT, TOUCH_GT911_RST,
                           TOUCH_PANEL_WIDTH, TOUCH_PANEL_HEIGHT);


static lv_indev_t *indev = nullptr;

bool touch_init(void) {
    Serial.println("[Touch] Initializing GT911...");
    Serial.printf("[Touch] Panel: %dx%d, Display: %dx%d\n", 
                 TOUCH_PANEL_WIDTH, TOUCH_PANEL_HEIGHT, 
                 DISPLAY_WIDTH, DISPLAY_HEIGHT);

    Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
    
    // Initialize GT911
    ts.begin();
    
    // Check if device responds
    Wire.beginTransmission(0x5D);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
        Serial.println("[Touch] GT911 detected on I2C bus");
    } else {
        Serial.printf("[Touch] ERROR: GT911 not responding (I2C error: %d)\n", error);
        return false;
    }
    
    // Set rotation
    ts.setRotation(ROTATION_INVERTED);
    
    // LVGL input device initialization
    indev = lv_indev_create();
    if (!indev) {
        Serial.println("[Touch] ERROR: Failed to create LVGL input device");
        return false;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read);

    Serial.println("[Touch] GT911 initialized successfully!");
    return true;
}

void touch_read(lv_indev_t *indev_drv, lv_indev_data_t *data) {
    static bool was_touched = false;
    static unsigned long last_debug = 0;
    
    // Read touch data
    ts.read();
    
    if (ts.isTouched && ts.touches > 0) {
        // Get raw touch coordinates
        int16_t raw_x = ts.points[0].x;  // Keep as signed to see the actual value
        int16_t raw_y = ts.points[0].y;
        
        
        // Handle negative values BEFORE mapping
        if (raw_x < 0) {
            Serial.printf("[Touch] WARNING: Negative X value detected: %d\n", raw_x);
            raw_x = 0;  // Clamp to minimum
        }
        if (raw_y < 0) {
            Serial.printf("[Touch] WARNING: Negative Y value detected: %d\n", raw_y);
            raw_y = 0;  // Clamp to minimum
        }
        
        // Clamp to touch panel boundaries
        raw_x = constrain(raw_x, 0, TOUCH_PANEL_WIDTH - 1);
        raw_y = constrain(raw_y, 0, TOUCH_PANEL_HEIGHT - 1);
        
        // Map to display coordinates (ONLY ONCE)
        data->point.x = map(raw_x, 0, TOUCH_PANEL_WIDTH - 1, 0, DISPLAY_WIDTH - 1);
        data->point.y = map(raw_y, 0, TOUCH_PANEL_HEIGHT - 1, 0, DISPLAY_HEIGHT - 1);
        
        data->state = LV_INDEV_STATE_PRESSED;

        // Reset screen timeout on touch
        if (!was_touched) {
            resetScreenTimeout();
            was_touched = true;
            
            Serial.printf("[Touch] Mapped: X=%d, Y=%d (Display: %dx%d)\n", 
                         data->point.x, data->point.y, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        was_touched = false;
    }
}