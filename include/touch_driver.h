#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <Arduino.h>
#include "lvgl.h"

// GT911 Touch Controller pins for ESP32-P4 JC4880P443C
#define TOUCH_GT911_SDA  7
#define TOUCH_GT911_SCL  8
#define TOUCH_GT911_INT  11   
#define TOUCH_GT911_RST  22   

// Function declarations
bool touch_init(void);
void touch_read(lv_indev_t *indev, lv_indev_data_t *data);



#endif // TOUCH_DRIVER_H
