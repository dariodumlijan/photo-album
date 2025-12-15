# ESP32 Photo Frame

A digital photo frame built with ESP32 that displays images from a MicroSD card on a 3.5" TFT touchscreen display. This project uses PlatformIO for development and dependency management.

## Hardware Requirements

- **Board**: ESP32-32E development board
- **Display**: [3.5" ESP32-32E Display with ST7796 driver](https://www.lcdwiki.com/3.5inch_ESP32-32E_Display)
  - Resolution: 480x320 pixels
  - Built-in resistive touch screen
  - ST7796 display controller
- **Storage**: 8GB MicroSD card (formatted as FAT32)
- **Power**: USB-C or 5V power supply

## Features

- 📸 Automatic slideshow with configurable interval (default: 5 seconds)
- 👆 Touch navigation (tap left/right to go backward/forward)
- 🖼️ Centered image display with aspect ratio preservation
- 🔄 Supports multiple image formats through preprocessing
- 📁 Reads all JPG images from SD card root directory

## Board Configuration

This project is specifically configured for the **3.5" ESP32-32E Display**. The configuration uses the following pins:

### TFT Display (HSPI bus)

- **MISO**: GPIO 12
- **MOSI**: GPIO 13
- **SCLK**: GPIO 14
- **CS**: GPIO 15
- **DC**: GPIO 2
- **RST**: -1 (connected to board reset)
- **Backlight**: GPIO 27

### Touch Screen

- **CS**: GPIO 33

### SD Card (VSPI bus)

- **MISO**: GPIO 19
- **MOSI**: GPIO 23
- **SCLK**: GPIO 18
- **CS**: GPIO 5

### Display Settings

- **Driver**: ST7796
- **SPI Frequency**: 80 MHz (display)
- **Touch Frequency**: 2.5 MHz
- **SPI Port**: HSPI (for TFT) / VSPI (for SD card)

> ⚠️ **Important**: The configuration files in the [`replace/`](./replace) directory must be copied to your TFT_eSPI library installation to match this specific hardware setup. See [TFT_eSPI Configuration](#3-configure-tft_espi-library) below.

## Software Dependencies

This project uses [PlatformIO](https://platformio.org/) for development and dependency management. The required libraries are automatically downloaded when you build the project.

### Required Libraries (managed by PlatformIO)

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) v2.5.43 - TFT display driver
- [TJpg_Decoder](https://github.com/Bodmer/TJpg_Decoder) v1.1.0 - JPEG decoder optimized for embedded systems

### Platform

- ESP32 (Espressif32 platform v6.5.0)
- Arduino Framework

## Setup Instructions

### 1. Install PlatformIO

Choose one of the following methods:

**Option A: VS Code Extension (Recommended)**

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the PlatformIO IDE extension from the VS Code marketplace
3. Restart VS Code

**Option B: PlatformIO Core (CLI)**

```bash
pip install -U platformio
```

### 2. Clone and Open the Project

```bash
git clone <your-repo-url>
cd photo-frame
```

If using VS Code with PlatformIO:

- Open the `photo-frame` folder in VS Code
- PlatformIO will automatically detect the project

### 3. Configure TFT_eSPI Library

**Critical Step**: The TFT_eSPI library requires hardware-specific configuration files. The [`replace/`](./replace) directory contains two essential configuration files that must be copied to your local TFT_eSPI library installation:

#### Configuration Files Explained

1. **[`replace/User_Setup.h`](./replace/User_Setup.h)** - Main TFT_eSPI configuration

   - Selects the **ST7796** display driver (line 59)
   - Configures **ESP32 pin assignments** for the 3.5" ESP32-32E Display:
     - TFT connections (HSPI bus): MISO=12, MOSI=13, SCLK=14, CS=15, DC=2, RST=-1
     - Touch screen CS: GPIO 33
     - Backlight control: GPIO 27
   - Sets **SPI frequency** to 80MHz for fast display updates (line 358)
   - Sets **touch frequency** to 2.5MHz for XPT2046 touch controller (line 364)
   - Enables **HSPI port** usage (line 369) so VSPI remains free for SD card
   - Loads required fonts for any text display needs

2. **[`replace/ST7796_Init.h`](./replace/ST7796_Init.h)** - Display initialization sequence
   - Contains the low-level command sequence to initialize the ST7796 controller
   - Configures display parameters: color mode, memory access control, power settings
   - Sets gamma correction curves for proper color reproduction
   - This initialization sequence is specifically tuned for the 3.5" ESP32-32E Display

#### Installation Steps

1. **Build the project once** to download libraries:

   ```bash
   pio run
   ```

2. **Locate your TFT_eSPI library directory:**

   - PlatformIO installs libraries in: `.pio/libdeps/esp32dev/TFT_eSPI/`
   - This is relative to your project directory

3. **Copy the configuration files:**

   ```bash
   # From the project root directory
   cp replace/User_Setup.h .pio/libdeps/esp32dev/TFT_eSPI/User_Setup.h
   cp replace/ST7796_Init.h .pio/libdeps/esp32dev/TFT_eSPI/TFT_Drivers/ST7796_Init.h
   ```

4. **Rebuild the project** to apply the configuration:
   ```bash
   pio run
   ```

> **Note**: You need to copy these files each time you clean the PlatformIO library cache or switch environments, as PlatformIO may re-download fresh library copies.

### 4. Prepare Your Images

The project includes a helper script to prepare images for optimal display:

```bash
./scripts/prepare.sh
```

#### What the `prepare.sh` Script Does

The [`prepare.sh`](./scripts/prepare.sh) script automates image preparation to ensure optimal performance on the ESP32:

1. **Resizes images** to fit the display (480x320 pixels)

   - Maintains original aspect ratio
   - Only shrinks images that are larger than the target size
   - Smaller images are left unchanged

2. **Converts all formats to JPG**

   - Input formats supported: JPG, JPEG, PNG, GIF, BMP, TIFF, WEBP, HEIC, HEIF
   - Output: High-quality JPG files (quality: 90)
   - Optimized for fast decoding on ESP32

3. **Organizes files**

   - Source: [`assets/root/`](./assets/root) - Put your original images here
   - Output: [`assets/target/`](./assets/target) - Optimized images ready for SD card

4. **Requirements**
   - **ImageMagick** must be installed:
     - macOS: `brew install imagemagick`
     - Ubuntu/Debian: `sudo apt-get install imagemagick`
     - Windows: Download from [imagemagick.org](https://imagemagick.org/script/download.php)

#### Usage

1. Place your images in [`assets/root/`](./assets/root)
2. Run the script: `./scripts/prepare.sh`
3. Copy processed images from [`assets/target/`](./assets/target) to your SD card root directory

#### Randomizing Image Display Order

The project includes a [`randomize.sh`](./scripts/randomize.sh) script to shuffle the display order of images:

```bash
./scripts/randomize.sh
```

**What the `randomize.sh` Script Does:**

- Renames all `.jpg` files in [`assets/target/`](./assets/target) with randomized names
- Creates a random display order (images are displayed alphabetically by filename)
- Files are renamed to sequential numbers (001.jpg, 002.jpg, etc.) in random order
- Useful for creating variety in slideshow presentation

**Usage:**

1. After running `prepare.sh`, run: `./scripts/randomize.sh`
2. The images in [`assets/target/`](./assets/target) will be renamed in random order
3. Copy the randomized images to your SD card
4. Each time you run the script, a new random order is generated

### 5. Prepare the SD Card

1. Format an 8GB MicroSD card as **FAT32**
2. Copy your prepared JPG images to the **root directory** of the SD card
3. Safely eject the card

### 6. Build and Upload the Code

**Using VS Code with PlatformIO:**

1. Connect your ESP32 board via USB
2. Click the **PlatformIO: Upload** button (→ icon) in the status bar
3. Or use the Command Palette (Ctrl+Shift+P): "PlatformIO: Upload"

**Using PlatformIO CLI:**

```bash
# Build the project
pio run

# Upload to the board (auto-detects port)
pio run --target upload

# Monitor serial output
pio device monitor

# Or combine upload and monitor
pio run --target upload && pio device monitor
```

**Specifying a port manually:**

```bash
pio run --target upload --upload-port /dev/ttyUSB0  # Linux
pio run --target upload --upload-port COM3          # Windows
pio run --target upload --upload-port /dev/cu.usbserial-*  # macOS
```

## Usage

### Basic Operation

1. Insert the MicroSD card with images
2. Power on the ESP32
3. The photo frame will automatically start displaying images

### Navigation

- **Auto-advance**: Images change every 5 seconds (configurable)
- **Touch left side**: Go to previous image
- **Touch right side**: Go to next image
- **Boot button**: Press to toggle display on/off (backlight control)

### Customization

Edit [`src/main.cpp`](./src/main.cpp:1) to customize:

```cpp
#define IMAGE_SWITCH_DELAY 5000 // Change slideshow interval (milliseconds)
```

## Touch Calibration

The touch screen is pre-calibrated for landscape mode. If you need to recalibrate:

1. The calibration sketch is in [`calibration/touch.cpp`](./calibration/touch.cpp)
2. Temporarily rename it to main.cpp or modify platformio.ini to use it
3. Upload to your ESP32 and follow on-screen instructions
4. Update the calibration data in [`src/main.cpp`](./src/main.cpp:122):

```cpp
uint16_t calData[5] = {257, 3677, 223, 3571, 7};
```

## Project Structure

```
.
├── platformio.ini            # PlatformIO project configuration
├── src/
│   └── main.cpp              # Main photo frame application
├── calibration/
│   └── touch.cpp             # Touch screen calibration utility
├── replace/
│   ├── README.md             # Configuration file installation guide
│   ├── User_Setup.h          # TFT_eSPI configuration for ESP32-32E Display
│   └── ST7796_Init.h         # ST7796 driver initialization sequence
├── scripts/
│   ├── prepare.sh            # Image preparation script
│   ├── randomize.sh          # Image randomization script
│   └── pipeline.sh           # Combined preparation and randomization
├── assets/
│   ├── root/                 # Place original images here
│   └── target/               # Processed images (copy to SD card)
├── include/                  # Header files (if needed)
├── lib/                      # Project-specific libraries
├── test/                     # Unit tests
└── README.md                 # This file
```

## Troubleshooting

### Display shows nothing

- Check TFT_eSPI configuration files were copied correctly
- Verify power supply provides sufficient current (500mA+)
- Check all pin connections

### SD card not detected

- Ensure card is formatted as FAT32
- Try a different SD card (some cards have compatibility issues)
- Check SD card pins are clean

### Images not displaying correctly

- Ensure images are in JPG format
- Run [`prepare.sh`](./scripts/prepare.sh) to resize and optimize images
- Check images are in the root directory of SD card

### Touch not working

- Run the calibration sketch in [`calibration/touch.cpp`](./calibration/touch.cpp)
- Update calibration values in [`src/main.cpp`](./src/main.cpp)

### Build or compilation errors

- Ensure you've copied the configuration files from [`replace/`](./replace) directory
- Clean the build and rebuild: `pio run --target clean && pio run`
- Check that the TFT_eSPI library was downloaded: verify `.pio/libdeps/esp32dev/TFT_eSPI/` exists

## Changelog

### Recent Updates

#### Image Preparation Pipeline Script

- Created new [`pipeline.sh`](./scripts/pipeline.sh) script that combines image preparation and randomization
- Automatically runs [`prepare.sh`](./scripts/prepare.sh) followed by [`randomize.sh`](./scripts/randomize.sh)
- Includes colored output for better visibility of pipeline stages
- Provides clear success/failure feedback for each step
- Aborts pipeline if any step fails

**Usage:**

```bash
./scripts/pipeline.sh
```

#### Image Preparation Script Enhancement

- Modified [`prepare.sh`](./scripts/prepare.sh) to automatically clean the target directory
- Before processing new images, removes all existing files in [`assets/target/`](./assets/target) (except `.keep` file)
- Ensures fresh output with each run, preventing duplicate or outdated images
- Provides user feedback during the cleaning process

## License

This project is open source. Feel free to modify and distribute.

## Credits

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) library by Bodmer
- [TJpg_Decoder](https://github.com/Bodmer/TJpg_Decoder) library by Bodmer
- Display information from [LCD Wiki](https://www.lcdwiki.com/3.5inch_ESP32-32E_Display)
