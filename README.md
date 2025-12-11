# ESP32 Photo Frame

A digital photo frame built with ESP32 that displays images from a MicroSD card on a 3.5" TFT touchscreen display.

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

> ⚠️ **Important**: The configuration files in the [`replace/`](./replace) directory must be copied to your TFT_eSPI library installation to match this specific hardware setup. See [Setup Instructions](#setup-instructions) below.

## Software Dependencies

### Arduino Libraries (install via Library Manager)

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) - TFT display driver
- [TJpg_Decoder](https://github.com/Bodmer/TJpg_Decoder) - JPEG decoder optimized for embedded systems

## Setup Instructions

### 1. Install Arduino IDE and ESP32 Board Support

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Go to **File → Preferences**
   - Add to "Additional Board Manager URLs": `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Go to **Tools → Board → Boards Manager**
   - Search for "esp32" and install "esp32 by Espressif Systems"

### 2. Install Required Libraries

Open Arduino IDE:

1. Go to **Sketch → Include Library → Manage Libraries**
2. Install:
   - **TFT_eSPI** by Bodmer
   - **TJpg_Decoder** by Bodmer

### 3. Configure TFT_eSPI Library

The TFT_eSPI library needs to be configured for the specific display hardware:

1. Locate your TFT_eSPI library installation directory:

   - **Windows**: `Documents\Arduino\libraries\TFT_eSPI\`
   - **macOS**: `~/Documents/Arduino/libraries/TFT_eSPI/`
   - **Linux**: `~/Arduino/libraries/TFT_eSPI/`

2. Copy configuration files from this project:

   ```bash
   cp replace/User_Setup.h <TFT_eSPI_dir>/User_Setup.h
   cp replace/ST7796_Init.h <TFT_eSPI_dir>/TFT_Drivers/ST7796_Init.h
   ```

3. Or manually update these files with the settings from the [`replace/`](./replace) directory

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

### 5. Prepare the SD Card

1. Format an 8GB MicroSD card as **FAT32**
2. Copy your prepared JPG images to the **root directory** of the SD card
3. Safely eject the card

### 6. Upload the Code

1. Open [`main/main.ino`](./main/main.ino) in Arduino IDE
2. Select your board:
   - **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
3. Select the correct port:
   - **Tools → Port → [your ESP32 port]**
4. Click **Upload** (or press Ctrl+U)

## Usage

### Basic Operation

1. Insert the MicroSD card with images
2. Power on the ESP32
3. The photo frame will automatically start displaying images

### Navigation

- **Auto-advance**: Images change every 5 seconds (configurable)
- **Touch left side**: Go to previous image
- **Touch right side**: Go to next image

### Customization

Edit [`main/main.ino`](./main/main.ino:1) to customize:

```cpp
#define IMAGE_SWITCH_DELAY 5000 // Change slideshow interval (milliseconds)
```

## Touch Calibration

The touch screen is pre-calibrated for landscape mode. If you need to recalibrate:

1. Open [`calibrate/calibrate.ino`](./calibrate/calibrate.ino)
2. Upload to your ESP32
3. Follow the on-screen instructions
4. Update the calibration data in [`main/main.ino`](./main/main.ino:62):

```cpp
uint16_t calData[5] = {257, 3677, 223, 3571, 7};
```

## Project Structure

```
.
├── main/
│   └── main.ino              # Main photo frame application
├── calibrate/
│   └── calibrate.ino         # Touch screen calibration utility
├── replace/
│   ├── User_Setup.h          # TFT_eSPI configuration for ESP32-32E Display
│   └── ST7796_Init.h         # ST7796 driver initialization sequence
├── scripts/
│   └── prepare.sh            # Image preparation script
├── assets/
│   ├── root/                 # Place original images here
│   └── target/               # Processed images (copy to SD card)
└── README.md
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

- Run the calibration sketch in [`calibrate/`](./calibrate)
- Update calibration values in main sketch

## License

This project is open source. Feel free to modify and distribute.

## Credits

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) library by Bodmer
- [TJpg_Decoder](https://github.com/Bodmer/TJpg_Decoder) library by Bodmer
- Display information from [LCD Wiki](https://www.lcdwiki.com/3.5inch_ESP32-32E_Display)
