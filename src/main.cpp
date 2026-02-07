/**
 * ESP32-P4 Sonos Controller
 * 480x800 MIPI DSI Display with Touch
 * Modern UI matching reference design
 */

#include "ui_common.h"
#include "config.h"
#include "lyrics.h"
#include <esp_flash.h>
#include <esp_task_wdt.h>

// Sonos logo
LV_IMG_DECLARE(Sonos_idnu60bqes_1);

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(500);
    Serial.println("\n=== SONOS CONTROLLER ===");
    Serial.printf("Free heap: %d, PSRAM: %d\n", esp_get_free_heap_size(), heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    // Detect flash chip - auto-suspend only works with specific chips
    uint32_t flash_id = 0;
    esp_err_t ret = esp_flash_read_id(esp_flash_default_chip, &flash_id);

    if (ret == ESP_OK) {
        uint8_t mfg_id = (flash_id >> 16) & 0xFF;
        uint8_t capacity_id = flash_id & 0xFF;
        int flash_size_mb = (1 << capacity_id) / (1024 * 1024);

        const char* mfg_name;
        switch(mfg_id) {
            case 0x68: mfg_name = "Boya BY25Q"; break;
            case 0xC8: mfg_name = "GigaDevice GD25"; break;
            case 0x20: mfg_name = "XMC XM25"; break;
            case 0xEF: mfg_name = "Winbond W25"; break;
            case 0x1C: mfg_name = "EON EN25"; break;
            case 0xA1: mfg_name = "Fudan FM25"; break;
            default:   mfg_name = "Unknown"; break;
        }

        // Check if flash supports auto-suspend (ESP-IDF: GD25QxxE, XM25QxxC, FM25Q32)
        bool suspend_ok = (mfg_id == 0xC8 || mfg_id == 0x20 || mfg_id == 0xA1);
        Serial.printf("[FLASH] %s %dMB (0x%06X) - Auto-suspend: %s\n",
                      mfg_name, flash_size_mb, flash_id, suspend_ok ? "YES" : "NO");
    }

    // Create network mutex to serialize WiFi access (prevents SDIO buffer overflow)
    network_mutex = xSemaphoreCreateMutex();

    // Create OTA progress mutex to protect OTA state and UI updates
    ota_progress_mutex = xSemaphoreCreateMutex();

    // Initialize preferences with debug logging
    wifiPrefs.begin(NVS_NAMESPACE, false);
    String ssid = wifiPrefs.getString(NVS_KEY_SSID, DEFAULT_WIFI_SSID);
    String pass = wifiPrefs.getString(NVS_KEY_PASSWORD, DEFAULT_WIFI_PASSWORD);

    // Debug: Log what was loaded from NVS
    if (ssid.length() > 0) {
        Serial.printf("[WIFI] Loaded from NVS: SSID='%s', (length: %d)\n", ssid.c_str(), pass.length());
    } else {
        Serial.println("[WIFI] No saved credentials found in NVS, using defaults");
    }

    // Load display settings from NVS
    brightness_level = wifiPrefs.getInt(NVS_KEY_BRIGHTNESS, DEFAULT_BRIGHTNESS);
    brightness_dimmed = wifiPrefs.getInt(NVS_KEY_BRIGHTNESS_DIM, DEFAULT_BRIGHTNESS_DIM);
    autodim_timeout = wifiPrefs.getInt(NVS_KEY_AUTODIM, DEFAULT_AUTODIM_SEC);
    lyrics_enabled = wifiPrefs.getBool(NVS_KEY_LYRICS, true);
    Serial.printf("[DISPLAY] Loaded settings from NVS: brightness=%d%%, dimmed=%d%%, autodim=%dsec, lyrics=%s\n",
                  brightness_level, brightness_dimmed, autodim_timeout, lyrics_enabled ? "on" : "off");

    // Brightness will be set after display_init() is called
    Serial.println("[DISPLAY] ESP32-P4 uses ST7701 backlight control (no PWM needed)");

    //Initialise WiFi Connection
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    lv_init();
    if (!display_init()) { Serial.println("Display FAIL"); while(1) delay(1000); }
    if (!touch_init()) { Serial.println("Touch FAIL"); while(1) delay(1000); }

    // Initialize hardware watchdog timer - auto-reboot if system hangs
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WATCHDOG_TIMEOUT_SEC * 1000,
        .idle_core_mask = 0,  // Don't watch idle tasks
        .trigger_panic = true // Reboot on timeout
    };
    esp_task_wdt_reconfigure(&wdt_config);
    esp_task_wdt_add(NULL);  // Add main task to watchdog
    Serial.printf("[WDT] Watchdog enabled: %d sec timeout\n", WATCHDOG_TIMEOUT_SEC);

    // Set initial brightness
    setBrightness(brightness_level);
    Serial.printf("[DISPLAY] Initial brightness: %d%%\n", brightness_level);

    // Show boot screen with Sonos logo
    lv_obj_t* boot_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(boot_scr, lv_color_hex(0x000000), 0);
    lv_screen_load(boot_scr);

    // Sonos logo (scale down significantly)
    lv_obj_t* img_logo = lv_image_create(boot_scr);
    lv_image_set_src(img_logo, &Sonos_idnu60bqes_1);
    lv_obj_align(img_logo, LV_ALIGN_CENTER, 0, -30);
    // Scale down significantly (256 = 100%, so 80 = ~31% size, 100 = ~39% size)
    lv_image_set_scale(img_logo, 130);  // Smaller - about 25% of original size

    // Create animated progress bar below logo
    lv_obj_t* boot_bar = lv_bar_create(boot_scr);
    lv_obj_set_size(boot_bar, 300, 8);
    lv_obj_align(boot_bar, LV_ALIGN_CENTER, 0, 80);
    lv_obj_set_style_bg_color(boot_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(boot_bar, lv_color_hex(0xD4A84B), LV_PART_INDICATOR);
    lv_obj_set_style_border_width(boot_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(boot_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(boot_bar, 4, LV_PART_INDICATOR);
    lv_bar_set_range(boot_bar, 0, 100);
    lv_bar_set_value(boot_bar, 0, LV_ANIM_OFF);

    // Helper to update boot progress
    auto updateBootProgress = [&](int percent) {
        lv_bar_set_value(boot_bar, percent, LV_ANIM_ON);
        lv_refr_now(NULL);
        lv_tick_inc(10);
        lv_timer_handler();
    };

    updateBootProgress(5);  // Initial display

    // Add global touch callback for screen wake
    lv_display_add_event_cb(lv_display_get_default(), [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_PRESSED) {
            resetScreenTimeout();
        }
    }, LV_EVENT_PRESSED, NULL);

    updateBootProgress(10);  // Callbacks ready

    // Initialize lyrics PSRAM buffer before creating screens
    initLyrics();

    createMainScreen();
    updateBootProgress(15);

    createDevicesScreen();
    updateBootProgress(20);

    createQueueScreen();
    updateBootProgress(22);

    createSettingsScreen();
    updateBootProgress(25);

    createDisplaySettingsScreen();
    updateBootProgress(30);
   
    // Connect to WiFi
    int retryCount = 0;
    int maxRetries = 6;
    

    while (retryCount < maxRetries && WiFi.status() != WL_CONNECTED) {
        Serial.printf("[WIFI] Attempt %d/%d to connect to '%s'\n", retryCount+1, maxRetries, ssid.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());
        
        unsigned long startAttemptTime = millis();
        bool connected = false;
        
        // Wait for connection with a timeout (e.g., 10 seconds)
        while (millis() - startAttemptTime < 10000) {
            if (WiFi.status() == WL_CONNECTED) {
                
                break;
            }
            delay(500);
            Serial.print(".");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("\n[WIFI] Connected successfully! IP: %s\n", WiFi.localIP().toString().c_str());
            
        } else {
            Serial.println("\n[WIFI] Attempt failed. Retrying...");
            retryCount++;
            delay(1000); // Wait a second before retrying
            updateBootProgress(30+(retryCount * 5)); // Shows system still alive while retrying
        }
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] All connection attempts failed.");
        }
    
    updateBootProgress(60);

    createWiFiScreen();
    updateBootProgress(62);

    createOTAScreen();
    updateBootProgress(65);

    createSourcesScreen();
    updateBootProgress(68);

    createGroupsScreen();
    updateBootProgress(70);

    art_mutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(albumArtTask, "Art", ART_TASK_STACK_SIZE, NULL, ART_TASK_PRIORITY, &albumArtTaskHandle, 0);
    updateBootProgress(90);

    sonos.begin();
    updateBootProgress(75);

    // Try to load cached device first for fast boot (~2s vs ~15s)
    bool loadedFromCache = sonos.tryLoadCachedDevice();
    if (loadedFromCache) {
        sonos.selectDevice(0);
        sonos.startTasks();
    } else {
        // No cache or unreachable - run full SSDP discovery
        int cnt = sonos.discoverDevices();
        if (cnt > 0) {
            sonos.selectDevice(0);
            sonos.startTasks();
        }
    }

    updateBootProgress(100);  // Complete!
    delay(300);  // Show 100% briefly

    lv_screen_load(scr_main);  // Now load main screen
    Serial.println("Ready!");
}

// WiFi auto-reconnection check (runs every 10 seconds when disconnected)
static unsigned long lastWifiCheck = 0;
static const unsigned long WIFI_CHECK_INTERVAL = 10000;  // 10 seconds

void checkWiFiReconnect() {
    if (millis() - lastWifiCheck < WIFI_CHECK_INTERVAL) return;
    lastWifiCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Connection lost, attempting reconnect...");
        WiFi.reconnect();
    }
}

// Periodic heap monitoring for debugging memory issues
static unsigned long lastHeapLog = 0;

void logHeapStatus() {
    if (millis() - lastHeapLog < HEAP_LOG_INTERVAL_MS) return;
    lastHeapLog = millis();

    size_t free_heap = esp_get_free_heap_size();
    size_t min_heap = esp_get_minimum_free_heap_size();
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    Serial.printf("[HEAP] Free: %dKB | Min: %dKB | PSRAM: %dKB\n",
                  free_heap / 1024, min_heap / 1024, free_psram / 1024);

    // Log task stack high water marks (unused stack space in words)
    // Lower number = more stack used, closer to overflow
    // Multiply by 4 to get bytes (ESP32 uses 4-byte words)
    Serial.printf("[STACK] Art:%d ", albumArtTaskHandle ? uxTaskGetStackHighWaterMark(albumArtTaskHandle) * 4 : 0);
    Serial.printf("Net:%d ", sonos.getNetworkTaskHandle() ? uxTaskGetStackHighWaterMark(sonos.getNetworkTaskHandle()) * 4 : 0);
    Serial.printf("Poll:%d bytes free\n", sonos.getPollingTaskHandle() ? uxTaskGetStackHighWaterMark(sonos.getPollingTaskHandle()) * 4 : 0);

    // Warn if heap is getting low
    if (free_heap < 50000) {
        Serial.println("[HEAP] WARNING: Low memory!");
    }
}

void loop() {
    // Feed watchdog to prevent reboot (must call regularly)
    esp_task_wdt_reset();

    lv_tick_inc(3);

    // Skip LVGL timer during OTA to prevent PSRAM access during flash writes
    bool skip_updates = false;
    if (xSemaphoreTake(ota_progress_mutex, pdMS_TO_TICKS(10))) {
        skip_updates = ota_in_progress;
        xSemaphoreGive(ota_progress_mutex);
    }

    if (!skip_updates) {
        lv_timer_handler();
        processUpdates();
        checkAutoDim();
        checkWiFiReconnect();
        logHeapStatus();  // Periodic memory monitoring
    }

    vTaskDelay(pdMS_TO_TICKS(3));
}
