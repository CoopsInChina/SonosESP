<div align="center">

# SonosESP | ESP32-P4 Sonos Controller

**A modern, touchscreen controller for Sonos speakers built with ESP32-P4**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Ready-blue.svg)](https://platformio.org/)
[![GitHub Downloads (all releases)](https://img.shields.io/github/downloads/CoopsInChina/SonosESP/total?style=flat-square&logo=github&label=Downloads)](https://github.com/CoopsInChina/SonosESP/releases)
[![GitHub Release](https://img.shields.io/github/v/release/CoopsInChina/SonosESP?style=flat-square&logo=github&label=Latest%20Release)](https://github.com/CoopsInChina/SonosESP/releases/latest)

[Features](#features) • [Hardware](#hardware) • [Installation](#installation) • [NFC Tags](#nfc-tags) • [Contributing](#contributing)


</div>

---

## Features

- **Full Playback Control** - Play, pause, skip, volume, shuffle, and repeat
- **Queue Management** - Browse and manage your playback queue
- **Album Art Display** - Hardware JPEG decoder + PNG support with bilinear scaling and automatic dominant color extraction
- **Synced Lyrics Display** - Time-synced lyrics from LRCLIB overlaid on album art with smart auto-hide, scroll effects, and color matching
- **NFC Tag Playback** - Tap an NTAG NFC tag to instantly clear the queue and play a Spotify or Apple Music album or playlist on any Sonos room
- **Clock Screensaver** - Full-screen clock activates after inactivity with random ambient background images, tap to dismiss
- **Photo Screensaver** - Local photos as clock screensaver backgrounds, embedded at build time via Python script for offline use without network dependency
- **Music Browsing** - Navigate your Sonos library, playlists, and favorites
- **Multi-Room** - Switch between Sonos zones with live playing indicators showing which rooms are active
- **OTA Updates** - Firmware updates from GitHub with Stable and Nightly release channel selection, auto-retry on low memory
- **Multi-Screen Support** - Supports both 4" 800×480 and 7" 1024×600 displays
- **Reliable Sonos Discovery** - Non-blocking deferred device discovery prevents UI freeze on startup; network mutex protection eliminates SDIO crash during concurrent WiFi operations
- **Weather Widget** - Live weather overlay on clock screensaver with temperature, humidity, wind speed, and conditions
- **Blurred Album Art Background** - Full-screen blurred background generated from album art for immersive experience

![SonosESP Demo](assets/image1.gif)

## Hardware

SonosESP supports two compatible development boards:

### 4" GUITION JC4880P433C
![GUITION JC4880P433C](assets/image.png)

| Component | Specification |
|-----------|--------------|
| **MCU** | ESP32-P4 (400 MHz dual-core) |
| **WiFi Module** | ESP32-C6 (via ESP-Hosted) |
| **Display** | 800×480 RGB LCD with ST7701 driver |
| **Touch** | GT911 capacitive touch (I2C) |
| **Flash** | 16 MB |
| **PSRAM** | OPI PSRAM |
| **Interface** | USB-C |

### 7" GUITION JC1060P470C
![GUITION JC1060P470C](assets/image2.webp)

| Component | Specification |
|-----------|--------------|
| **MCU** | ESP32-P4 (400 MHz dual-core) |
| **WiFi Module** | ESP32-C6 (via ESP-Hosted) |
| **Display** | 1024×600 RGB LCD with JD9165 driver |
| **Touch** | GT911 capacitive touch (I2C) |
| **Flash** | 16 MB |
| **PSRAM** | OPI PSRAM |
| **Interface** | USB-C |

> **Note:** The firmware includes conditional compilation for both screen sizes. Select the correct build environment in PlatformIO (`esp32_4inch` or `esp32_7inch`) for your hardware.

### NFC Reader (optional)
The NFC feature requires a **PN532 NFC module** connected via I2C. Any standard PN532 breakout board works. Configure the I2C pins in `include/config.h` to match your wiring.

Tags must be **NTAG213, NTAG215, or NTAG216** (NDEF-capable NTAG series). Pre-formatted NTAG tags are widely available and inexpensive.


## Installation

### Web Installer (Recommended)
1. Visit the [Web Installer](https://CoopsInChina.github.io/SonosESP/)
2. Select your screen size (4" or 7")
3. Connect your ESP32-P4 via USB-C
4. Click "Install Firmware" and select the COM port
5. Wait for installation to complete
6. Configure WiFi using the on-screen keyboard after reboot

> Requires Chrome, Edge, or Opera browser with Web Serial support

### Manual Build with PlatformIO
For developers or custom configurations:

```bash
# Clone the repository
git clone -b release https://github.com/CoopsInChina/SonosESP.git
cd SonosESP

# Build for 4" screen
pio run -e esp32_4inch

# Build for 7" screen
pio run -e esp32_7inch

# Upload to device
pio run -e esp32_4inch -t upload
```

## OTA Updates (After Initial Install)
The device supports automatic Over-The-Air (OTA) firmware updates from GitHub releases:

1. Connect to WiFi via Settings
2. Navigate to Settings → Firmware Update
3. Tap "Check for Updates"
4. If an update is available, tap "Install Update"
5. Device will automatically download and install the correct firmware for your screen size

## First-Time Setup
1. **Power on** - Device will show WiFi setup if not configured
2. **WiFi Setup** - Tap "Scan" to find networks, select yours, enter password
3. **Sonos Discovery** - Navigate to Settings → Speakers and tap "Scan"
4. **Start Playing** - Select a device and start controlling your music!

## NFC Tags

### Prerequisites
- PN532 NFC module wired to the ESP32-P4 I2C pins
- [node-sonos-http-api](https://github.com/jishi/node-sonos-http-api) running on your local network (e.g. on a Raspberry Pi)
- Apple Music or Spotify linked to your Sonos system via the Sonos app

On first boot with the NFC module connected, the device will automatically scan your local network for node-sonos-http-api (port 5005) and cache its address. The NFC screen under Settings shows discovery status.

### Programming a tag

Tags store a plain NDEF text record in the format `service:type:id`. Use any NFC writing app (e.g. **NFC Tools** on iOS/Android) to write a plain text NDEF record.

#### URI format

| Content | URI format | Example |
|---------|-----------|---------|
| Spotify album | `spotify:album:<ID>` | `spotify:album:2dfTV7CktUEBkZCHiB7VQB` |
| Spotify playlist | `spotify:playlist:<ID>` | `spotify:playlist:32O0SSXDNWDrMievPkV0Im` |
| Apple Music album | `applemusic:album:<ID>` | `applemusic:album:1443155637` |
| Apple Music playlist | `applemusic:playlist:<ID>` | `applemusic:playlist:pl.abcdef123` |

#### Finding the ID

**Spotify:** Right-click an album or playlist → Share → Copy Link. The ID is the last segment of the URL:
```
https://open.spotify.com/album/2dfTV7CktUEBkZCHiB7VQB
                                      ^^^^^^^^^^^^^^^^^^^^ this is the ID
```

**Apple Music:** Right-click → Share → Copy Link. The numeric ID appears in the URL:
```
https://music.apple.com/au/album/songs-in-the-key-of-life/1443155637
                                                            ^^^^^^^^^^ this is the ID
```

#### Writing the tag with NFC Tools (iOS/Android)
1. Open NFC Tools → Write → Add a record → Text
2. Enter the URI (e.g. `spotify:album:2dfTV7CktUEBkZCHiB7VQB`)
3. Tap **Write** and hold your tag to the phone

#### How it works when tapped
1. Tag is read → NDEF text extracted
2. Current Sonos queue is cleared
3. Album or playlist is sent to node-sonos-http-api which tells Sonos to play it
4. Playback starts in the room selected on the NFC settings screen

## Photo Screensaver — Custom Photos

The screensaver background photos are embedded into the firmware at build time by `scripts/embed_photos.py`. The repository ships with generic placeholder images. You can replace them with your own photos.

### Using your own photos

Place three JPEG files named `Photo1.jpg`, `Photo2.jpg`, and `Photo3.jpg` into the appropriate directory for your screen size:

```
assets/
  4inchScreensavers/   ← photos for 4" builds (800×480 recommended)
  7inchScreensavers/   ← photos for 7" builds (1024×600 recommended)
```

Replace the existing files and rebuild. The script will embed them automatically.

### Keeping photos private (local builds only)

If you want to use personal photos locally without ever committing them to git, create a `PrivateAssets` folder inside `assets/`:

```
assets/
  PrivateAssets/
    4inchScreensavers/
      Photo1.jpg
      Photo2.jpg
      Photo3.jpg
    7inchScreensavers/
      Photo1.jpg
      Photo2.jpg
      Photo3.jpg
```

The build script automatically detects `assets/PrivateAssets/` and uses those photos instead of the public ones. The `PrivateAssets/` folder is listed in `.gitignore` and will never be committed — CI and public builds always use the generic photos from `assets/{size}inchScreensavers/`.

## Architecture

### Key Components
- **FreeRTOS Tasks** - Separate tasks for UI, album art, lyrics, and Sonos polling
- **Thread Safety** - Mutex protection for shared resources
- **Memory Management** - PSRAM for album art and lyrics, heap monitoring
- **Network Layer** - HTTPClient for SOAP requests, HTTPS for lyrics/art, UDP for SSDP discovery
- **UI Framework** - LVGL 9.4.0 with custom theme
- **Image Processing** - ESP32-P4 hardware JPEG decoder + software PNG decoder, custom bilinear scaling with fixed-point math
- **Lyrics System** - Time-synced LRC parsing with HTTPS fetching, auto-hide, and retry logic
- **Clock Screensaver** - Inactivity-triggered fullscreen clock with random background photos and weather widget
- **OTA Updates** - Stable and Nightly channels, 3-attempt retry loop with live countdown UI
- **Multi-Screen Support** - Conditional compilation for different display sizes and touch calibrations
- **NFC System** - PN532 I2C reader with background subnet discovery for node-sonos-http-api, NDEF parsing, and queue-clear-then-play flow

## Configuration
WiFi credentials are stored persistently in NVS (Non-Volatile Storage). Once configured via the UI, they survive reboots and power cycles.

### Firmware Updates
- Automatic OTA updates from GitHub releases
- Separate firmware assets per screen size (`firmware-4inch.bin` / `firmware-7inch.bin`)
- Version checking on demand
- Progress indication during download
- Safe rollback on failure

## Known Issues
See the [GitHub Issues](https://github.com/CoopsInChina/SonosESP/issues) for current open issues.

## Thanks

This project is a fork of [SonosESP](https://github.com/OpenSurface/SonosESP) by [Opensurface](https://github.com/OpenSurface). All credit for the original concept, architecture, and implementation goes to them and their contributors:

- **[@pizza-init](https://github.com/pizza-init)** - Lead maintainer and feature developer
- **[@BaileyLawson](https://github.com/BaileyLawson)** - First external contributor
- **[@johnhenrick3-cpu](https://github.com/johnhenrick3-cpu)** - Outstanding community tester

The NFC tap-to-play concept was inspired by **[@hankhank10](https://github.com/hankhank10)**'s [Vinyl Emulator](https://github.com/hankhank10/vinylemulator) for Raspberry Pi.


## License
This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments
- Built with [LVGL](https://lvgl.io/) - Amazing embedded graphics library
- [PlatformIO](https://platformio.org/) - Best embedded development platform
- [LRCLIB](https://lrclib.net/) - Free synced lyrics API
- [Unsplash](https://unsplash.com/) - Beautiful random background photos for the clock screensaver
- Sonos UPnP/SOAP API documentation and community
- Open-Meteo for free weather API
