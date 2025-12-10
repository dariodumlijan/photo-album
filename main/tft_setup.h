// 3.5inch ESP32-32E Display Setup for TFT_eSPI
// LCD Wiki: https://www.lcdwiki.com/3.5inch_ESP32-32E_Display
// This file will be automatically included by TFT_eSPI library

#ifndef USER_SETUP_LOADED
#define USER_SETUP_LOADED

// Define the driver for ST7796
#define ST7796_DRIVER

// Display resolution
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// ESP32-32E Specific Pin Configuration
// These pins are fixed on the ESP32-32E board
#define TFT_MISO 12  // SDO
#define TFT_MOSI 13  // SDI
#define TFT_SCLK 14  // SCK
#define TFT_CS   15  // CS
#define TFT_DC   2   // DC/RS
#define TFT_RST  -1  // RST connected to ESP32 EN pin (use -1)

// Backlight control - ESP32-32E has backlight on GPIO 27
#define TFT_BL   27
#define TFT_BACKLIGHT_ON HIGH

// SPI Frequency
#define SPI_FREQUENCY  27000000  // 27MHz - stable for most ST7796 displays
#define SPI_READ_FREQUENCY  20000000  // 20MHz for reading
#define SPI_TOUCH_FREQUENCY  2500000  // 2.5MHz for touch (if applicable)

// Color order - ESP32-32E uses BGR color order
#define TFT_RGB_ORDER TFT_BGR

// Fonts to be available
#define LOAD_GLCD  // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2 // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4 // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6 // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7 // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8 // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// Smooth fonts
#define SMOOTH_FONT

// JPEG decoder
#define SUPPORT_TRANSACTIONS

#endif // USER_SETUP_LOADED