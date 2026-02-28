/**
 * config.h - Centralized configuration constants
 * All magic numbers and configurable values in one place
 */

#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// SERIAL & DEBUG
// =============================================================================
#define SERIAL_BAUD_RATE        115200

// Debug levels: 0=OFF, 1=ERRORS, 2=WARNINGS, 3=INFO, 4=VERBOSE
#define DEBUG_LEVEL             3

// Debug macros - compile out verbose logs when not needed
#if DEBUG_LEVEL >= 4
    #define DEBUG_VERBOSE(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_VERBOSE(fmt, ...) ((void)0)
#endif

#if DEBUG_LEVEL >= 3
    #define DEBUG_INFO(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_INFO(fmt, ...) ((void)0)
#endif

#if DEBUG_LEVEL >= 2
    #define DEBUG_WARN(fmt, ...) Serial.printf("[WARN] " fmt, ##__VA_ARGS__)
#else
    #define DEBUG_WARN(fmt, ...) ((void)0)
#endif

#if DEBUG_LEVEL >= 1
    #define DEBUG_ERROR(fmt, ...) Serial.printf("[ERROR] " fmt, ##__VA_ARGS__)
#else
    #define DEBUG_ERROR(fmt, ...) ((void)0)
#endif

// =============================================================================
// WIFI CONFIGURATION
// =============================================================================
#define WIFI_INIT_DELAY_MS      2000    // Delay for ESP32-C6 SDIO initialization
#define WIFI_CONNECT_TIMEOUT_MS 500     // Per-attempt timeout
#define WIFI_CONNECT_RETRIES    40      // Max connection attempts (40 x 500ms = 20s)
#define WIFI_MAX_NETWORKS       20      // Max networks to scan/store
#define WIFI_CHECK_INTERVAL_MS  10000   // Main loop: WiFi health check interval

// =============================================================================
// DISPLAY SETTINGS
// =============================================================================
#define DEFAULT_BRIGHTNESS      100     // Default brightness percentage
#define DEFAULT_BRIGHTNESS_DIM  20      // Default dimmed brightness
#define DEFAULT_AUTODIM_SEC     30      // Default auto-dim timeout (seconds)
#define MIN_BRIGHTNESS          5       // Minimum brightness allowed
#define MAX_BRIGHTNESS          100     // Maximum brightness

// Display dimensions (LVGL renders in landscape, driver rotates to portrait panel)
#define DISPLAY_WIDTH           800     // LVGL width (landscape)
#define DISPLAY_HEIGHT          480     // LVGL height (landscape)
#define PANEL_WIDTH             480     // Physical panel width (portrait)
#define PANEL_HEIGHT            800     // Physical panel height (portrait)

// =============================================================================
// ALBUM ART
// =============================================================================
#define ART_MAX_DOWNLOAD_SIZE   (280 * 1024)  // Max JPEG download buffer (280KB)
#define ART_TASK_STACK_SIZE     20000   // Album art task stack — PNG decode stacks TLS + software
                                        // decoder on same task; 12KB hit stack=0 on HTTPS PNG (BBC R4)
#define ART_TASK_PRIORITY       0       // Album art task priority
#define ART_DOWNLOAD_TIMEOUT_MS 8000    // Download timeout
#define ART_CHECK_INTERVAL_MS   100     // How often to check for new art requests
#define ART_DECODE_MAX_FAILURES 3       // Give up on URL after N decode failures
#define ART_SW_JPEG_FALLBACK    1       // Enable JPEGDEC SW fallback (progressive, non-div-8)

// =============================================================================
// SONOS CONTROLLER
// =============================================================================
#define SONOS_MAX_DEVICES       32      // Maximum discoverable devices (keep in sync with MAX_SONOS_DEVICES)
#define SONOS_QUEUE_SIZE_MAX    500     // Maximum queue items to fetch
#define SONOS_QUEUE_BATCH_SIZE  50      // Items per queue fetch request
#define SONOS_CMD_QUEUE_SIZE    10      // Command queue depth
#define SONOS_UI_QUEUE_SIZE     20      // UI update queue depth

// Task configuration (stack sizes in BYTES — ESP-IDF xTaskCreate takes bytes, not words)
// High water marks (before fix) showed Poll=684 bytes free / 3000 total = DANGER.
// sendSOAP chain can push 600-800 more bytes → overflow on deeper paths (queue, media info).
// Net=1504 bytes free / 3500 total — also tight. Both doubled for safety.
#define SONOS_NET_TASK_STACK    6000    // Network task stack size (was 3500; actual free was ~1.5KB)
#define SONOS_POLL_TASK_STACK   6000    // Polling task stack size (was 3000; actual free was ~684 bytes!)
#define SONOS_NET_TASK_PRIORITY 2       // Network task priority
#define SONOS_POLL_TASK_PRIORITY 3      // Polling task priority
#define LYRICS_TASK_STACK       8192    // Lyrics task stack size (4096 overflowed on HTTPS fetch — WiFiClientSecure + URL[512] needs ~6KB)
#define LYRICS_TASK_PRIORITY    1       // Lyrics task priority
// Arduino loopTask stack is hard-coded to 8KB in pre-compiled framework (sdkconfig.h) — cannot
// be overridden with -D flags. mainAppTask runs the actual UI loop with a proper stack.
// loopTask becomes idle (vTaskDelay only). Watchdog transfers to mainAppTask.
// MUST be internal SRAM: NVS writes call spi_flash_disable_interrupts_caches_and_other_cpu()
// which asserts (esp_task_stack_is_sane_cache_disabled) if the calling task's stack is in PSRAM.
// 16KB is ample — HWM shows < 5KB used. Art task in PSRAM already frees 20KB DMA SRAM headroom.
#define MAIN_APP_TASK_STACK     16384   // mainAppTask stack — internal SRAM (flash/NVS safe)
#define MAIN_APP_TASK_PRIORITY  1       // Same priority as loopTask; Sonos (2/3) preempts as before

// Timeouts
#define SONOS_SOAP_TIMEOUT_MS   2000    // SOAP request timeout
#define SONOS_DEBOUNCE_MS       400     // Command debounce time

// Polling tick modulos (base interval = 300ms, so N ticks = N * 300ms)
#define POLL_VOLUME_MODULO      5       // Volume every 1.5s (5 * 300ms)
#define POLL_TRANSPORT_MODULO   10      // Transport settings every 3s
#define POLL_QUEUE_MODULO       100     // Queue every 30s (optimized: was 50/15s)
#define POLL_MEDIA_INFO_MODULO  50      // Radio station info every 15s
#define POLL_BASE_INTERVAL_MS   300     // Base polling interval

// =============================================================================
// OTA UPDATES
// =============================================================================
#define OTA_BUFFER_SIZE         2048    // Download buffer (static in main loop — not FreeRTOS stack)
#define OTA_READ_SIZE           2048    // Max bytes per read. 2KB: same adaptive delay, 2× throughput.
                                        // Critical zone (DMA<4KB): 2KB+80ms = 25KB/s vs old 12.5KB/s
                                        // → ~40s download instead of ~68s for 2MB firmware
#define OTA_MAX_FIRMWARE_SIZE   (10 * 1024 * 1024)  // 10MB max firmware
#define OTA_DOWNLOAD_TIMEOUT_MS 300000  // 5 minutes max for entire download
#define OTA_STALL_TIMEOUT_MS    30000   // 30 seconds max with no data received
#define OTA_PROGRESS_LOG_INTERVAL 10    // Log every N percent
#define OTA_DMA_CHECK_INTERVAL  4       // Check DMA pressure every N chunks (~4KB)
#define OTA_DMA_CRITICAL        4096    // DMA critical threshold (80ms delay)
#define OTA_DMA_LOW             8192    // DMA low threshold (30ms delay)
#define OTA_BASE_DELAY_MS       15      // Base per-chunk delay (~65KB/s, ~25s for 1.5MB)
#define OTA_TLS_MAX_RETRIES     3             // Retry full connect+download on connection failure
#define OTA_TLS_RETRY_DELAY_MS  5000          // Wait between retry attempts (ms per attempt)
#define OTA_MIN_DMA_AFTER_TLS   (8 * 1024)   // Min total free DMA after TLS GET completes.
                                              // < 8KB → SDIO RX pool starved → assert crash.
                                              // Normal full handshake leaves ~17-21KB (safe).
                                              // heap_caps_get_largest_free_block(MALLOC_CAP_DMA)
                                              // always returns 0 on ESP32-P4, so total only.
#define OTA_TARGET_FREE_DMA     (112 * 1024) // Min free DMA before attempting the OTA TLS handshake.
                                              // Lowered 120→112KB: the post-TLS check (OTA_MIN_DMA_AFTER_TLS)
                                              // is the real SDIO safety gate. At 112KB pre-TLS:
                                              //   attempt 1: 112-106=6KB post-TLS → fails 8KB check → retry
                                              //   TLS cleanup returns ~106KB → DMA recovers to ~117KB
                                              //   attempt 2: 117-106=11KB → passes → downloads (no reboot!)
                                              // Typical running-app DMA: 110-112KB. Was always rebooting;
                                              // now handles borderline cases with one extra TLS retry (~5s).
                                              // Genuinely low cases (<110KB) still reboot via plateau detection.
#define OTA_HTTPS_COOLDOWN_MS   2000    // Wait for previous HTTPS cleanup
#define OTA_CHECK_DEBOUNCE_MS   5000    // Min delay between update checks
#define OTA_CHECK_TIMEOUT_MS    15000   // HTTP timeout for version check
#define OTA_CHECK_CLEANUP_MS    500     // Delay after version check TLS cleanup
#define OTA_DMA_POLL_MS         15000   // Max wait for TIME_WAIT sockets to expire
                                        // lwIP TIME_WAIT = 2×MSL ≈ 12s; 15s gives 3s margin
#define OTA_DMA_PLATEAU_COUNT   3       // Consecutive seconds with no DMA growth → reboot early

// =============================================================================
// MBEDTLS / SSL
// =============================================================================
#define MBEDTLS_SSL_IN_LEN      4096    // SSL input buffer size
#define MBEDTLS_SSL_OUT_LEN     4096    // SSL output buffer size

// =============================================================================
// NVS PREFERENCES
// =============================================================================
#define NVS_NAMESPACE           "sonos_wifi"
#define NVS_KEY_SSID            "ssid"
#define NVS_KEY_PASSWORD        "pass"
#define NVS_KEY_BRIGHTNESS      "brightness"
#define NVS_KEY_BRIGHTNESS_DIM  "brightness_dimmed"
#define NVS_KEY_AUTODIM         "autodim_sec"
#define NVS_KEY_OTA_CHANNEL     "ota_channel"
#define NVS_KEY_CACHED_DEVICE   "cached_dev"
#define NVS_KEY_LYRICS          "lyrics"
#define NVS_KEY_OTA_PENDING     "ota_pending"    // Auto-reboot OTA flag
#define NVS_KEY_OTA_URL         "ota_url"        // Saved firmware URL for auto-reboot OTA

// =============================================================================
// UI COLORS (hex values)
// =============================================================================
#define COLOR_BACKGROUND        0x000000
#define COLOR_TEXT_PRIMARY      0xFFFFFF
#define COLOR_TEXT_SECONDARY    0x888888
#define COLOR_ACCENT            0xD4A84B  // Sonos gold
#define COLOR_SUCCESS           0x00FF00
#define COLOR_ERROR             0xFF0000
#define COLOR_WARNING           0xFFA500

// =============================================================================
// CLOCK / SCREENSAVER
// =============================================================================
#define CLOCK_MODE_DISABLED    0  // Never show clock screen
#define CLOCK_MODE_INACTIVITY  1  // Show after X mins no touch (any play state)
#define CLOCK_MODE_PAUSED      2  // Show only when paused/stopped + X mins inactivity
#define CLOCK_MODE_NOTHING     3  // Show only when nothing playing + X mins inactivity

#define CLOCK_DEFAULT_MODE       0    // Disabled by default
#define CLOCK_DEFAULT_TIMEOUT    5    // 5 minutes inactivity before clock
#define CLOCK_DEFAULT_TZ_IDX     0    // Index 0 = UTC
#define CLOCK_DEFAULT_PICSUM     1    // Enable random photo background
#define CLOCK_DEFAULT_REFRESH    10   // Refresh background every 10 minutes
#define CLOCK_DEFAULT_KW_IDX     0    // Index 0 = Random (no keyword)
#define CLOCK_DEFAULT_12H        0    // 0 = 24h, 1 = 12h
#define CLOCK_DEFAULT_WEATHER_EN   1  // Weather widget enabled by default
#define CLOCK_DEFAULT_WEATHER_CITY 0  // 0 = Auto-detect from IP
#define CLOCK_WX_REFRESH_MIN      15  // Re-fetch weather every 15 min (independent of photo rate)
#define CLOCK_DEFAULT_WEATHER_FAHR 0  // 0 = Celsius, 1 = Fahrenheit

#define CLOCK_BG_MAX_DL_SIZE  (512 * 1024)  // Max background JPEG download buffer (512KB; Flickr baseline ~100-250KB)
#define CLOCK_BG_WIDTH        800           // Clock background pixel width
#define CLOCK_BG_HEIGHT       480           // Clock background pixel height
#define CLOCK_BG_TASK_STACK   8192          // clockBgTask stack size
#define CLOCK_ENTER_TIMEOUT_MS 3000         // Max wait for art/lyrics tasks to exit
#define CLOCK_EXIT_COOLDOWN_MS 30000        // Prevent re-trigger for 30s after exit

#define NVS_KEY_CLOCK_MODE      "clk_mode"
#define NVS_KEY_CLOCK_TIMEOUT   "clk_timeout"
#define NVS_KEY_CLOCK_TZ        "clk_tz"
#define NVS_KEY_CLOCK_PICSUM    "clk_picsum"
#define NVS_KEY_CLOCK_REFRESH   "clk_refresh"
#define NVS_KEY_CLOCK_KW        "clk_kw"
#define NVS_KEY_CLOCK_12H       "clk_12h"
#define NVS_KEY_CLOCK_WEATHER_EN   "clk_wx_en"
#define NVS_KEY_CLOCK_WEATHER_CITY "clk_wx_city"
#define NVS_KEY_CLOCK_WEATHER_FAHR "clk_wx_fahr"

// =============================================================================
// QUEUE / PLAYLIST
// =============================================================================
#define QUEUE_ADD_AT_END        4294967295  // Add to end of queue constant

// =============================================================================
// WATCHDOG & RELIABILITY
// =============================================================================
#define WATCHDOG_TIMEOUT_SEC    30          // Watchdog timeout (device reboots if stuck)
#define HEAP_LOG_INTERVAL_MS    60000       // Log heap status every 60 seconds

#endif // CONFIG_H
