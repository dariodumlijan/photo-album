#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <JPEGDecoder.h>

// TFT display instance
TFT_eSPI tft = TFT_eSPI();

// SD card chip select pin for ESP32-32E board
// The ESP32-32E board uses GPIO 4 for SD card CS
#define SD_CS 4

// Backlight pin (defined in tft_setup.h but referenced here for control)
#define TFT_BL 27

// Display settings
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

// Image display duration (milliseconds)
#define IMAGE_DISPLAY_TIME 5000

// Array to store image filenames
String imageFiles[100];
int imageCount = 0;
int currentImageIndex = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("ESP32-32E Photo Frame Starting...");

  // Initialize backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // Turn on backlight

  // Initialize TFT display
  tft.init();
  tft.setRotation(1); // Landscape mode (adjust 0-3 as needed)
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  // Display startup message
  tft.setCursor(10, 150);
  tft.println("Initializing SD Card...");

  // Initialize SD card
  if (!SD.begin(SD_CS))
  {
    Serial.println("SD Card initialization failed!");
    tft.fillScreen(TFT_RED);
    tft.setCursor(10, 150);
    tft.println("SD Card Error!");
    while (1)
    {
      delay(1000);
    }
  }

  Serial.println("SD Card initialized successfully");
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 150);
  tft.println("SD Card OK!");
  delay(1000);

  // Scan for JPG files
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 150);
  tft.println("Scanning for images...");

  scanSDCard();

  if (imageCount == 0)
  {
    Serial.println("No JPG images found on SD card!");
    tft.fillScreen(TFT_YELLOW);
    tft.setCursor(10, 150);
    tft.println("No JPG files found!");
    while (1)
    {
      delay(1000);
    }
  }

  Serial.print("Found ");
  Serial.print(imageCount);
  Serial.println(" images");

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 150);
  tft.print("Found ");
  tft.print(imageCount);
  tft.println(" images!");
  delay(2000);

  tft.fillScreen(TFT_BLACK);
}

void loop()
{
  // Display current image
  if (imageCount > 0)
  {
    displayImage(imageFiles[currentImageIndex]);

    // Move to next image
    currentImageIndex++;
    if (currentImageIndex >= imageCount)
    {
      currentImageIndex = 0; // Loop back to first image
    }

    // Wait before showing next image
    delay(IMAGE_DISPLAY_TIME);
  }
}

void scanSDCard()
{
  File root = SD.open("/");
  if (!root)
  {
    Serial.println("Failed to open root directory");
    return;
  }

  imageCount = 0;

  File file = root.openNextFile();
  while (file && imageCount < 100)
  {
    if (!file.isDirectory())
    {
      String fileName = String(file.name());
      fileName.toLowerCase();

      // Check if file is a JPG
      if (fileName.endsWith(".jpg") || fileName.endsWith(".jpeg"))
      {
        imageFiles[imageCount] = String(file.name());
        Serial.print("Found image: ");
        Serial.println(imageFiles[imageCount]);
        imageCount++;
      }
    }
    file.close();
    file = root.openNextFile();
  }

  root.close();
}

void displayImage(String filename)
{
  Serial.print("Displaying: ");
  Serial.println(filename);

  // Clear screen
  tft.fillScreen(TFT_BLACK);

  // Open the file
  File jpegFile = SD.open("/" + filename);
  if (!jpegFile)
  {
    Serial.println("Failed to open image file");
    tft.setCursor(10, 150);
    tft.println("Error loading image");
    return;
  }

  // Decode the JPEG
  bool decoded = JpegDec.decodeSdFile(jpegFile);

  if (!decoded)
  {
    Serial.println("JPEG decode failed");
    jpegFile.close();
    tft.setCursor(10, 150);
    tft.println("Decode error");
    return;
  }

  // Render the image
  renderJPEG(0, 0);

  jpegFile.close();
}

void renderJPEG(int xpos, int ypos)
{
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  // Calculate centering offsets
  int32_t offset_x = (SCREEN_WIDTH - max_x) / 2;
  int32_t offset_y = (SCREEN_HEIGHT - max_y) / 2;

  if (offset_x < 0)
    offset_x = 0;
  if (offset_y < 0)
    offset_y = 0;

  // JPEG images are decoded in blocks (MCUs)
  while (JpegDec.read())
  {
    pImg = JpegDec.pImage;

    int mcu_x = JpegDec.MCUx * mcu_w + xpos + offset_x;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos + offset_y;

    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;

    // Clip the window to the image bounds
    if (mcu_x + mcu_w > max_x + offset_x)
      win_w = max_x + offset_x - mcu_x;
    if (mcu_y + mcu_h > max_y + offset_y)
      win_h = max_y + offset_y - mcu_y;

    // Skip if outside screen bounds
    if (mcu_x >= SCREEN_WIDTH || mcu_y >= SCREEN_HEIGHT)
      continue;

    // Clip to screen bounds
    if (mcu_x + win_w > SCREEN_WIDTH)
      win_w = SCREEN_WIDTH - mcu_x;
    if (mcu_y + win_h > SCREEN_HEIGHT)
      win_h = SCREEN_HEIGHT - mcu_y;

    // Draw the MCU block
    if (win_w > 0 && win_h > 0)
    {
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    }
  }
}