# TFT_eSPI Configuration Files

This directory contains hardware-specific configuration files for the TFT_eSPI library that must be copied to your local TFT_eSPI installation.

## Quick Reference

| File            | Source                  | Destination                                                | Purpose                                           |
| --------------- | ----------------------- | ---------------------------------------------------------- | ------------------------------------------------- |
| `User_Setup.h`  | `replace/User_Setup.h`  | `.pio/libdeps/esp32dev/TFT_eSPI/User_Setup.h`              | Pin configuration, driver selection, SPI settings |
| `ST7796_Init.h` | `replace/ST7796_Init.h` | `.pio/libdeps/esp32dev/TFT_eSPI/TFT_Drivers/ST7796_Init.h` | Display initialization commands                   |

## Why These Files Are Needed

The TFT_eSPI library is a flexible display driver that supports many different display controllers and pin configurations. To work with our specific hardware (3.5" ESP32-32E Display with ST7796 controller), the library needs to be configured with:

1. The correct display driver selection
2. Proper GPIO pin assignments
3. Appropriate SPI bus and frequency settings
4. Display initialization sequence optimized for this hardware

## Files in This Directory

### 1. `User_Setup.h`

**Purpose**: Main configuration file for TFT_eSPI library

**Target Location**: `.pio/libdeps/esp32dev/TFT_eSPI/User_Setup.h`

**What it configures**:

- **Display Driver Selection** (Line 59): Enables `ST7796_DRIVER` for the 3.5" display
- **ESP32 Pin Assignments** (Lines 205-223):

  ```cpp
  #define TFT_MISO 12    // Display MISO
  #define TFT_MOSI 13    // Display MOSI
  #define TFT_SCLK 14    // Display clock
  #define TFT_CS   15    // Display chip select
  #define TFT_DC   2     // Display data/command
  #define TFT_RST  -1    // Reset (connected to board reset)
  #define TFT_BL   27    // Backlight control
  #define TOUCH_CS 33    // Touch controller chip select
  ```

- **SPI Bus Configuration**:

  - Line 358: `SPI_FREQUENCY 80000000` - 80MHz for fast display updates
  - Line 364: `SPI_TOUCH_FREQUENCY 2500000` - 2.5MHz for XPT2046 touch
  - Line 369: `USE_HSPI_PORT` - Uses HSPI bus, leaving VSPI free for SD card

- **Font Loading** (Lines 303-310): Enables various font sizes for text display

**Key Features**:

- High-speed 80MHz SPI for smooth image rendering
- Dedicated SPI buses (HSPI for display, VSPI for SD card) to avoid conflicts
- Touch screen support with proper frequency for reliable operation

### 2. `ST7796_Init.h`

**Purpose**: Display controller initialization sequence

**Target Location**: `.pio/libdeps/esp32dev/TFT_eSPI/TFT_Drivers/ST7796_Init.h`

**What it configures**:

This file contains the low-level command sequence sent to the ST7796 display controller during initialization. It sets:

- **Sleep Mode Exit** (Line 10-12): Wakes the display from sleep mode
- **Memory Access Control** (Lines 14-15): Configures display orientation and color order
- **Pixel Format** (Lines 17-18): Sets 16-bit RGB565 color mode (0x55)
- **Command Set Control** (Lines 20-24): Unlocks extended commands for configuration
- **Display Inversion** (Line 26-27): Controls color inversion behavior
- **Power Control** (Lines 36-47): Voltage regulators and power supply settings
- **VCOM Control** (Line 46-47): Adjusts contrast and viewing angle
- **Gamma Correction** (Lines 59-89): Two curves (positive/negative) for accurate colors
- **Display On** (Line 101): Enables the display output

**Why this matters**:

- Without the correct initialization sequence, the display may show incorrect colors, have poor contrast, or not work at all
- These commands are specifically tuned for the ESP32-32E Display hardware
- Gamma curves ensure images look natural and colors are accurate

## Installation Instructions

These files must be copied **after** building the project for the first time, as PlatformIO downloads libraries during the initial build.

### Step-by-Step Process

1. **First build** (downloads libraries):

   ```bash
   pio run
   ```

2. **Copy configuration files**:

   ```bash
   # From the project root directory
   cp replace/User_Setup.h .pio/libdeps/esp32dev/TFT_eSPI/User_Setup.h
   cp replace/ST7796_Init.h .pio/libdeps/esp32dev/TFT_eSPI/TFT_Drivers/ST7796_Init.h
   ```

3. **Rebuild** (applies configuration):
   ```bash
   pio run
   ```

### When to Copy Again

You need to re-copy these files if:

- You run `pio run --target clean` (cleans the build but libraries remain)
- You delete the `.pio` directory
- You update the TFT_eSPI library version in `platformio.ini`
- You switch to a different PlatformIO environment
- PlatformIO re-downloads the library for any reason

## Verification

To verify the configuration was applied correctly:

1. **Check for ST7796 in build output**: Look for `ST7796_DRIVER` in compilation messages
2. **Display test**: Upload the code - you should see the display initialize with no flickering
3. **Touch test**: Touch should respond accurately to left/right taps
4. **SD card test**: Images should load without SPI conflicts

## Troubleshooting

**Problem**: Display shows wrong colors or artifacts

- **Solution**: Verify `ST7796_Init.h` was copied correctly

**Problem**: Display doesn't initialize (stays white/black)

- **Solution**: Check that `User_Setup.h` has correct pin definitions

**Problem**: Touch not working

- **Solution**: Ensure `TOUCH_CS 33` is defined in `User_Setup.h`

**Problem**: SD card conflicts with display

- **Solution**: Verify `USE_HSPI_PORT` is uncommented in `User_Setup.h`

**Problem**: Need to copy files after every clean

- **Solution**: This is expected PlatformIO behavior. Consider creating a script to automate copying.

## Additional Resources

- [TFT_eSPI Documentation](https://github.com/Bodmer/TFT_eSPI)
- [ST7796 Datasheet](https://www.displayfuture.com/Display/datasheet/controller/ST7796s.pdf)
- [3.5" ESP32-32E Display Wiki](https://www.lcdwiki.com/3.5inch_ESP32-32E_Display)
