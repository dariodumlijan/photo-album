# TFT_eSPI Configuration Files

This directory contains hardware-specific configuration files for the TFT_eSPI library that must be copied to your local TFT_eSPI installation.

## Quick Reference

| File            | Source                  | Destination                                               | Purpose                                           |
| --------------- | ----------------------- | --------------------------------------------------------- | ------------------------------------------------- |
| `User_Setup.h`  | `replace/User_Setup.h`  | `~/<your_library_dir>/TFT_eSPI/User_Setup.h`              | Pin configuration, driver selection, SPI settings |
| `ST7796_Init.h` | `replace/ST7796_Init.h` | `~/<your_library_dir>/TFT_eSPI/TFT_Drivers/ST7796_Init.h` | Display initialization commands                   |

## Why These Files Are Needed

The TFT_eSPI library is a flexible display driver that supports many different display controllers and pin configurations. To work with our specific hardware (3.5" ESP32-32E Display with ST7796 controller), the library needs to be configured with:

1. The correct display driver selection
2. Proper GPIO pin assignments
3. Appropriate SPI bus and frequency settings
4. Display initialization sequence optimized for this hardware

## Additional Resources

- [TFT_eSPI Documentation](https://github.com/Bodmer/TFT_eSPI)
- [ST7796 Datasheet](https://www.displayfuture.com/Display/datasheet/controller/ST7796s.pdf)
- [3.5" ESP32-32E Display Wiki](https://www.lcdwiki.com/3.5inch_ESP32-32E_Display)
