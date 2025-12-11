#include <SPI.h>
#include "SD.h"
#include "FS.h"
#include <XPT2046_Touchscreen.h>
#include <TJpg_Decoder.h>

#include <TFT_eSPI.h> // Hardware-specific library

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

#define TFT_GREY 0x5AEB // New colour

// ESP32-32E Pin Definitions
// TFT and Touch share HSPI bus
#define TFT_CS 15
#define TOUCH_CS 33
#define TOUCH_IRQ 36

// SD Card uses VSPI bus
#define SD_CS 5

// HSPI pins (for TFT and Touch)
#define HSPI_MISO 12
#define HSPI_MOSI 13
#define HSPI_SCK 14

// VSPI pins (for SD Card)
#define VSPI_MISO 19
#define VSPI_MOSI 23
#define VSPI_SCK 18

// SPI control macros
#define SPI_ON_TFT digitalWrite(TFT_CS, LOW)
#define SPI_OFF_TFT digitalWrite(TFT_CS, HIGH)
#define SPI_ON_SD digitalWrite(SD_CS, LOW)
#define SPI_OFF_SD digitalWrite(SD_CS, HIGH)
#define TOUCH_ON digitalWrite(TOUCH_CS, LOW)
#define TOUCH_OFF digitalWrite(TOUCH_CS, HIGH)

// Touch screen using XPT2046
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

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

String file_list[40];
int file_num = 0;
int file_index = 0;

void setup(void)
{
  Serial.begin(115200);
  Serial.println("ESP32-32E Photo Frame Starting...");

  // Initialize chip select pins
  pinMode(SD_CS, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(TFT_CS, OUTPUT);

  // Disable all devices initially
  SPI_OFF_SD;
  TOUCH_OFF;
  SPI_OFF_TFT;

  // IMPORTANT: Initialize TFT BEFORE SD Card to avoid SPI conflicts
  // TFT_eSPI library will handle its own SPI setup based on tft_setup.h
  SPI_ON_TFT;
  tft.init();
  tft.setRotation(1); // Landscape mode
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
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
  file_num = get_pic_list(SD, "/", 0, file_list);
  Serial.print("JPG file count: ");
  Serial.println(file_num);
  Serial.println("All JPG files:");
  for (int i = 0; i < file_num; i++)
  {
    Serial.println(file_list[i]);
  }
  SPI_OFF_SD;

  // Initialize Touch Screen
  // XPT2046 will use the SPI pins defined in tft_setup.h (same as TFT)
  TOUCH_ON;
  touch.begin();
  touch.setRotation(1); // Match TFT rotation
  TOUCH_OFF;

  Serial.println("Initialization complete!");
}

long int runtime = millis();
int flag = 1;

void loop()
{
  // Auto-advance images every 10 seconds or when flag is set
  if ((millis() - runtime > 10000) || flag == 1)
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
    if (file_index >= file_num)
    {
      file_index = 0;
    }
    runtime = millis();
    flag = 0;
  }

  // Check for touch input
  TOUCH_ON;
  if (touch.touched())
  {
    TS_Point p = touch.getPoint();
    TOUCH_OFF;

    // XPT2046 raw values are typically 0-4095
    // Map to screen coordinates (480x320 in landscape)
    int touch_x = map(p.x, 300, 3800, 0, 480);
    int touch_y = map(p.y, 300, 3800, 0, 320);

    // Clamp values to screen bounds
    touch_x = constrain(touch_x, 0, 480);
    touch_y = constrain(touch_y, 0, 320);

    Serial.print("Touch at X: ");
    Serial.print(touch_x);
    Serial.print(", Y: ");
    Serial.println(touch_y);

    // Right half = next image, Left half = previous image
    if (touch_x > 240)
    {
      Serial.println("Next image");
      flag = 1;
    }
    else
    {
      Serial.println("Previous image");
      file_index = (file_index + file_num - 2) % file_num;
      flag = 1;
    }

    // Wait for touch release
    delay(200);
    while (touch.touched())
    {
      delay(10);
    }
  }
  else
  {
    TOUCH_OFF;
  }

  delay(50); // Small delay to prevent excessive polling
}

// Gets all image files in the SD card root directory
int get_pic_list(fs::FS &fs, const char *dirname, uint8_t levels, String wavlist[30])
{
  Serial.printf("Listing directory: %s\n", dirname);
  int i = 0;

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return i;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return i;
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
        if (temp.endsWith(".jpg") || temp.endsWith(".jpeg"))
        {
          wavlist[i] = file.name();
          Serial.print("Found: ");
          Serial.println(wavlist[i]);
          i++;
        }
      }
    }
    file = root.openNextFile();
  }
  return i;
}
