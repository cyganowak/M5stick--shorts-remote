# Shorts Remote v1.1.0 - ESP32 BLE Mouse Controller

A Bluetooth LE Mouse controller for M5StickC Plus2 that controls TikTok/YouTube Shorts on Android devices.

## Features

- **Skip Forward** - Tap Button A to skip to next video
- **Skip Backward** - Hold Button A to go to previous video
- **Pause/Play** - Tap Button B to toggle playback
- **Like** - Double-tap Button B to like the video
- **Speed Up** - Hold Button B to play at 2x speed
- **Auto-Dim** - Screen dims to 10% brightness after 10 seconds of inactivity (saves battery)
- **Battery Indicator** - Shows real-time battery level at bottom of screen
- **Connection Status** - Visual indicator shows pairing/connected status

## Hardware Requirements

- M5StickC Plus2 (ESP32-based)
- USB-C cable for flashing
- Android device with Bluetooth LE support

## Installation

### Prerequisites

1. Arduino IDE installed
2. ESP32 board support installed:
   - Add `https://espressif.github.io/arduino-esp32/package_esp32_index.json` to Additional Boards Manager URLs
   - Install "ESP32" by Espressif Systems
3. Install M5Stack board package
4. Connect M5StickC Plus2 via USB

### Upload Sketch

1. Open `shorts-remote.ino` in Arduino IDE
2. Select board: **M5Stack-StickCPlus2**
3. Select port: Your M5StickC's COM port
4. Click **Upload**

### Bluetooth Pairing

1. Power on M5StickC Plus2
2. Go to Android Settings → Bluetooth
3. Search for "Shorts Remote"
4. Pair without pairing key (no PIN needed)

## Usage

| Action | Button | Description |
|--------|--------|-------------|
| Skip Forward | A (tap) | Swipe up to skip current video |
| Skip Backward | A (hold 700ms) | Swipe down to previous video |
| Pause/Play | B (tap) | Toggle video playback |
| Like | B (double-tap) | Double-tap center of screen to like |
| Speed Up | B (hold 400ms) | Hold to play at 2x speed |

## Battery Management

- Screen updates battery level every 5 seconds
- Auto-dim feature reduces power consumption by dimming screen to 10% after 10 seconds of inactivity
- Touch any button to restore full brightness

## Configuration

Edit these constants in the sketch to configure it for your device:

```cpp
const int STEP_PX          = 60;   // Cursor movement per step
const int TAP_RIGHT_STEPS  = 1;    // Horizontal offset for button tap
const int TAP_DOWN_STEPS   = 1;    // Vertical offset for button tap
const int SWIPE_START_DOWN = 2;    // Skip starts 120px from top
const int SWIPE_STEPS      = 12;   // Number of swipe steps
const int SWIPE_DELAY      = 10;   // Delay between swipe steps
const int A_HOLD_MS        = 700;  // Button A hold duration for previous
const int B_HOLD_MS        = 400;  // Button B hold duration for speed
```

## Technical Details

- **Library**: ESP32-BLE-Mouse (T-vK)
- **Connection**: Bluetooth LE (not classic Bluetooth)
- **Screen**: 240x135 ST7789 display
- **Power**: 3.7V LiPo (internal) or USB power

## Troubleshooting

### Device not connecting
- Ensure device is in pairing mode (power cycle)
- Check Android Bluetooth settings
- Remove old pairings and re-pair

### Buttons not working
- Verify cursor positions are correct for your device
- Adjust `STEP_PX`, `TAP_RIGHT_STEPS`, `TAP_DOWN_STEPS` values
- Use `slamCorner()` to reset cursor to top-left

### Battery not updating
- Battery level updates every 5 seconds
- Touch any button to force immediate update
- Check battery level is being read correctly

## Version History

- **v1.1.0** - Added auto-dim feature, improved battery indicator, 5-second update interval
- **v1.0.8** - Fixed battery bar calculation, real-time updates
- **v1.0.7** - Lowered battery indicator position, v1.0.7 display
- **v1.0.6** - Added battery indicator, improved UI readability
- **v1.0.0** - Initial release with skip/pause/like/speed controls

## License

MIT License - feel free to modify and share!

## Credits

Based on ESP32-BLE-Mouse library by T-vK
