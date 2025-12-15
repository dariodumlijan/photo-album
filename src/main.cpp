#define IMAGE_SWITCH_DELAY 5000 // delay between images in milliseconds

#include <Arduino.h>
#include <SPI.h>
#include "SD.h"
#include "FS.h"
#include <TJpg_Decoder.h>
#include <vector>

#include <TFT_eSPI.h> // Hardware-specific library with built-in touch support

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

#define TFT_GREY 0x5AEB // New colour

#define BOOT_BUTTON 0       // GPIO0 is the boot button
#define BUTTON_DEBOUNCE 300 // milliseconds
#define TFT_BL 27           // Backlight control pin

// SD Card uses VSPI bus
#define SD_CS 5

// VSPI pins (for SD Card)
#define VSPI_MISO 19
#define VSPI_MOSI 23
#define VSPI_SCK 18

// SPI control macros
#define SPI_ON_SD digitalWrite(SD_CS, LOW)
#define SPI_OFF_SD digitalWrite(SD_CS, HIGH)

// Gets all image files in the SD card root directory
void get_pic_list(fs::FS &fs, const char *dirname, uint8_t levels, std::vector<String> &wavlist)
{
  Serial.printf("Listing directory: %s\n", dirname);
  wavlist.clear(); // Clear any existing entries

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      // Skip directories
    }
    else
    {
      String temp = file.name();

      // Skip hidden files (starting with .)
      if (!temp.startsWith(".") && !temp.startsWith("/."))
      {
        // Check if file is JPG
        temp.toLowerCase();
        if (temp.endsWith(".jpg"))
        {
          wavlist.push_back(file.name());
          Serial.print("Found: ");
          Serial.println(wavlist.back());
        }
      }
    }
    file = root.openNextFile();
  }
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
  // Stop further decoding as image is running off bottom of screen
  if (y >= tft.height())
    return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}

std::vector<String> file_list;
int file_index = 0;
bool display_on = true;
unsigned long last_button_press = 0;

void setup(void)
{
  Serial.begin(115200);
  Serial.println("ESP32-32E Photo Frame Starting...");

  // Initialize boot button
  pinMode(BOOT_BUTTON, INPUT_PULLUP);

  // Initialize backlight pin
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // Turn backlight on at startup

  // Initialize chip select pin for SD
  pinMode(SD_CS, OUTPUT);
  SPI_OFF_SD;

  // Initialize TFT (TFT_eSPI handles its own SPI and touch setup)
  tft.init();
  tft.setRotation(1); // Landscape mode
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

  // Calibrate touch for rotation 1 (landscape)
  uint16_t calData[5] = {257, 3677, 223, 3571, 7};
  tft.setTouch(calData);

  Serial.println("TFT and Touch initialized");

  // Initialize TJpg_Decoder
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);

  // Initialize VSPI for SD Card (separate bus from TFT)
  SPI.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, SD_CS);

  // Initialize SD Card on VSPI
  SPI_ON_SD;
  if (!SD.begin(SD_CS, SPI, 4000000))
  {
    Serial.println("SD Card Mount Failed!");
    SPI_OFF_SD;
    while (1)
      delay(1000);
  }
  Serial.println("SD Card Mount Succeeded");

  // Get list of JPG files
  get_pic_list(SD, "/", 0, file_list);
  Serial.print("JPG file count: ");
  Serial.println(file_list.size());

  SPI_OFF_SD;

  Serial.println("Initialization complete!");
}

unsigned long runtime = 0;
bool flag = true;

void loop()
{
  // Check boot button (with debouncing)
  if (digitalRead(BOOT_BUTTON) == LOW &&
      (millis() - last_button_press > BUTTON_DEBOUNCE))
  {

    display_on = !display_on; // Toggle state

    if (display_on)
    {
      digitalWrite(TFT_BL, HIGH); // Turn backlight on
      tft.fillScreen(TFT_BLACK);  // Clear any artifacts
    }
    else
    {
      tft.fillScreen(TFT_BLACK); // Fill screen black before turning off
      digitalWrite(TFT_BL, LOW); // Turn backlight off
    }

    last_button_press = millis();

    // Wait for button release
    while (digitalRead(BOOT_BUTTON) == LOW)
    {
      delay(50);
    }

    // Additional debounce after release
    delay(200);
  }

  // Auto-advance images every x seconds or when flag is set (only when display is on)
  if (display_on && ((millis() - runtime >= IMAGE_SWITCH_DELAY) || flag))
  {
    // Let TFT_eSPI manage CS internally
    tft.fillScreen(TFT_BLACK);

    // Add "/" prefix to filename for SD card path
    String filepath = "/" + file_list[file_index];

    bool image_rendered = false;

    // Wrap image rendering with error handling
    // Get image dimensions to center it
    uint16_t img_w = 0, img_h = 0;
    SPI_ON_SD;
    int result = TJpgDec.getFsJpgSize(&img_w, &img_h, filepath.c_str(), SD);

    if (result == 0)
    {
      // Calculate centered position
      int16_t x_pos = (tft.width() - img_w) / 2;
      int16_t y_pos = (tft.height() - img_h) / 2;

      // Ensure position is not negative
      if (x_pos < 0)
        x_pos = 0;
      if (y_pos < 0)
        y_pos = 0;

      // Try to draw the image
      result = TJpgDec.drawSdJpg(x_pos, y_pos, filepath.c_str());

      if (result == 0)
      {
        image_rendered = true;
      }
      else
      {
        Serial.print("Error drawing image (error code: ");
        Serial.print(result);
        Serial.println("). Skipping to next image.");
      }
    }
    else
    {
      Serial.print("Error getting image size (error code: ");
      Serial.print(result);
      Serial.println("). Skipping to next image.");
    }

    SPI_OFF_SD;

    file_index++;
    if (file_index >= file_list.size())
    {
      file_index = 0;
    }
    runtime = millis();
    flag = false;
  }

  // Check for touch input using TFT_eSPI's built-in touch support
  uint16_t touch_x = 0, touch_y = 0;

  // getTouch returns true if touch is detected, threshold filters light touches
  if (tft.getTouch(&touch_x, &touch_y, 600))
  {
    // Left side = previous image, Right side = next image
    // Screen is 480x320 in landscape mode (rotation 1)
    if (touch_x > 240)
    {
      flag = true;
    }
    else
    {
      file_index = (file_index + file_list.size() - 2) % file_list.size();
      flag = true;
    }

    // Wait for touch release with debounce
    delay(300);
    while (tft.getTouch(&touch_x, &touch_y, 600))
    {
      delay(50);
    }

    // Additional debounce after release
    delay(200);
  }

  delay(50); // Small delay to prevent excessive polling
}
