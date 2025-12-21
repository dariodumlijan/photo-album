# ESP32 Digital Photo Album

A digital photo album built with ESP32 that displays images from a MicroSD card on a 3.5" TFT touchscreen display.

![featured](.github/docs/featured.gif)

## Hardware Requirements

- **Board with Display**: [ESP32-32E with 3.5" ST7796 Display](https://www.lcdwiki.com/3.5inch_ESP32-32E_Display)
  - Resolution: 480x320 pixels
  - Built-in resistive touch screen
- **Storage**: MicroSD card (formatted as FAT32)
- **Power**: USB-C or 5V power supply

## Features

- 📸 **Automatic slideshow** with configurable intervals via settings screen
  - **interval options**: 10 sec, 30 sec, 1 min, 2 min, 5 min, 10 min, 15 min, 30 min, 45 min, 1 hour, or OFF (manual mode)
- 🔆 **Brightness control** with adjustable levels (10%-100%) via settings screen
  - PWM-based backlight control for smooth brightness adjustment
  - Settings persist during runtime
- 👆 **Enhanced touch navigation** with three distinct areas:
  - **Left third**: Previous image
  - **Center third**: Double-tap to open settings
  - **Right third**: Next image
- 🖼️ **Centered image display** with aspect ratio preservation
- 🔄 **Supports multiple image formats** through preprocessing
- 📁 **Reads all JPG images** from SD card root directory

## Board Configuration

This project is specifically configured for the **3.5" ESP32-32E Display**.

Make sure to adjust the display driver and pin assignments in the TFT_eSPI library configuration files based on your hardware.

> ⚠️ **Important**: The configuration files in the [`replace/`](./replace) directory must be copied to your TFT_eSPI library installation to match this specific hardware setup. See [TFT_eSPI Configuration](./replace/README.md).

## Software Dependencies

This project uses [PlatformIO](https://platformio.org/) for development and dependency management. The required libraries are automatically downloaded when you build the project.

### Required Libraries (managed by PlatformIO)

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) v2.5.43 - TFT display driver
- [TJpg_Decoder](https://github.com/Bodmer/TJpg_Decoder) v1.1.0 - JPEG decoder optimized for embedded systems

### Platform

- ESP32 (Espressif32 platform v6.5.0)
- Arduino Framework

## Prepare Your Images

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

#### Prepare the SD Card

1. Format a MicroSD card as **FAT32**
2. Copy your prepared JPG images to the **root directory** of the SD card
3. Safely eject the card

## Basic Operation

1. Build and Upload the Code
2. Insert the MicroSD card with images
3. Power on the ESP32
4. The photo frame will automatically start displaying images

### Touch Controls (Three-Area System)

The touchscreen is divided into three functional areas:

- **Left third (0-160px)**: Navigate to **previous image**
- **Center third (160-320px)**: **Double-tap to open settings screen**
- **Right third (320-480px)**: Navigate to **next image**

### Physical Controls

- **Boot button**: Press to toggle display on/off (backlight control)

### User Configuration (via Settings Screen)

- **Slideshow timing**: Double-tap center area to open settings screen, use +/- buttons to adjust interval
- **Manual mode**: Select "OFF" interval to disable auto-advance for manual-only navigation
- **Display brightness**: Adjust brightness from 10% to 100% in 10% increments via settings screen

### Developer Configuration

Edit [`src/main.cpp`](./src/main.cpp:1) to customize behavior constants:

```cpp
// Image delay configurations: {delay, label}
ImageDelayConfig delay_configs[] = {
    {10000, "10 sec"},
    {30000, "30 sec"},
    {60000, "1 min"},
    {120000, "2 min"},
    {300000, "5 min"},
    {600000, "10 min"},
    {900000, "15 min"},
    {1800000, "30 min"},
    {2700000, "45 min"},
    {3600000, "1 h"},
    {0, "OFF"} // manual mode (infinite delay)
};

// Button and touch behavior settings
#define BUTTON_DEBOUNCE 300       // Button debounce time in milliseconds
#define TOUCH_DEBOUNCE 150        // Touch debounce time
#define MULTI_TAP_WINDOW 500      // Time window for detecting double-tap (milliseconds)

// Screen area config
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320
#define TOUCH_SECTIONS SCREEN_WIDTH / 3
#define CENTER_TOUCH_LEFT TOUCH_SECTIONS * 1  // Left boundary of center area
#define CENTER_TOUCH_RIGHT TOUCH_SECTIONS * 2 // Right boundary of center area

// Default brightness (10-100 in steps of 10)
int current_brightness_pct = 100;
```

## License

This project is open source. Feel free to modify and distribute.

## Credits

Project inspired by [this post](https://www.instructables.com/Make-a-Digital-Photo-Album-by-ESP32/) from Lan_Makerfabs.
