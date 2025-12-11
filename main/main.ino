#define IMAGE_SWITCH_DELAY 10000 // delay between images in milliseconds

#include <SPI.h>
#include "SD.h"
#include "FS.h"
#include <TJpg_Decoder.h>
#include <vector>

#include <TFT_eSPI.h> // Hardware-specific library

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

#define TFT_GREY 0x5AEB // New colour

// ESP32-32E Pin Definitions
#define TFT_CS 15

// SD Card uses VSPI bus
#define SD_CS 5

// VSPI pins (for SD Card)
#define VSPI_MISO 19
#define VSPI_MOSI 23
#define VSPI_SCK 18

// SPI control macros
#define SPI_ON_TFT digitalWrite(TFT_CS, LOW)
#define SPI_OFF_TFT digitalWrite(TFT_CS, HIGH)
#define SPI_ON_SD digitalWrite(SD_CS, LOW)
#define SPI_OFF_SD digitalWrite(SD_CS, HIGH)

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

void setup(void)
{
  Serial.begin(115200);
  Serial.println("ESP32-32E Photo Frame Starting...");

  // Initialize chip select pins
  pinMode(SD_CS, OUTPUT);
  pinMode(TFT_CS, OUTPUT);

  // Disable all devices initially
  SPI_OFF_SD;
  SPI_OFF_TFT;

  // IMPORTANT: Initialize TFT BEFORE SD Card to avoid SPI conflicts
  // TFT_eSPI library will handle its own SPI setup based on tft_setup.h
  SPI_ON_TFT;
  tft.init();
  tft.setRotation(1); // Landscape mode
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

  // Set touch calibration for landscape mode (rotation 1)
  // These values should be calibrated using the Touch_calibrate example
  // For landscape mode (rotation 1): approximate calibration values
  uint16_t calData[5] = {286, 3534, 283, 3600, 6};
  tft.setTouch(calData);

  SPI_OFF_TFT;

  // Initialize TJpg_Decoder
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);

  // Initialize default SPI for SD Card (VSPI: pins 18, 19, 23)
  SPI.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, SD_CS);

  // Initialize SD Card
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
  // Auto-advance images every x seconds or when flag is set
  if ((millis() - runtime >= IMAGE_SWITCH_DELAY) || flag)
  {
    Serial.print("Displaying image: ");
    Serial.println(file_list[file_index]);

    SPI_ON_TFT;
    tft.fillScreen(TFT_BLACK);
    SPI_OFF_TFT;

    // Add "/" prefix to filename for SD card path
    String filepath = "/" + file_list[file_index];

    SPI_ON_SD;
    TJpgDec.drawSdJpg(0, 0, filepath.c_str());
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

  // getTouch returns true if touch is detected and coordinates are valid
  // The threshold of 600 helps filter out spurious touches
  if (tft.getTouch(&touch_x, &touch_y, 600))
  {
    Serial.print("Touch detected - X: ");
    Serial.print(touch_x);
    Serial.print(", Y: ");
    Serial.println(touch_y);

    // Right half = next image, Left half = previous image
    // Screen is 480x320 in landscape mode (rotation 1)
    if (touch_x > 240)
    {
      Serial.println("Next image");
      flag = true;
    }
    else
    {
      Serial.println("Previous image");
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
