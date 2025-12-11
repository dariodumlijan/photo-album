#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <TJpg_Decoder.h>

// TFT display instance
TFT_eSPI tft = TFT_eSPI();

// SD card pins for ESP32-32E board (uses separate VSPI bus)
#define SD_CS   5   // SD card chip select
#define SD_MOSI 23  // SD card MOSI
#define SD_MISO 19  // SD card MISO
#define SD_SCK  18  // SD card SCK

// TFT CS pin (from tft_setup.h)
#define TFT_CS  15

// Backlight pin (defined in tft_setup.h but referenced here for control)
#define TFT_BL 27

// SPI Chip Select Control Macros
// These ensure only one device is active at a time
#define SPI_ON_TFT  digitalWrite(TFT_CS, LOW)
#define SPI_OFF_TFT digitalWrite(TFT_CS, HIGH)
#define SPI_ON_SD   digitalWrite(SD_CS, LOW)
#define SPI_OFF_SD  digitalWrite(SD_CS, HIGH)

// Display settings
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

// Image display duration (milliseconds)
#define IMAGE_DISPLAY_TIME 5000

// Array to store image filenames
String imageFiles[100];
int imageCount = 0;
int currentImageIndex = 0;

// TJpg_Decoder callback function to output image to TFT
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  // Stop further decoding as image is running off bottom of screen
  if (y >= tft.height()) return 0;
  
  // Enable TFT, disable SD for display operation
  SPI_OFF_SD;
  SPI_ON_TFT;
  
  // This function will clip automatically at TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);
  
  // Disable TFT after operation
  SPI_OFF_TFT;
  
  // Return 1 to decode next block
  return 1;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("ESP32-32E Photo Frame Starting...");

  // Initialize chip select pins
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  
  // Initially disable both devices
  SPI_OFF_TFT;
  SPI_OFF_SD;

  // Initialize backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // Turn on backlight

  // Enable TFT for initialization
  SPI_ON_TFT;
  
  // Initialize TFT display
  tft.init();
  tft.setRotation(1); // Landscape mode (adjust 0-3 as needed)
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  // Display startup message
  tft.setCursor(10, 150);
  tft.println("Initializing SD Card...");
  
  // Disable TFT before SD operations
  SPI_OFF_TFT;

  // Initialize TJpg_Decoder
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  // Enable SD for initialization
  SPI_ON_SD;
  
  // Initialize SD card with custom SPI pins (VSPI bus)
  SPIClass spi = SPIClass(VSPI);
  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
  // Disable SD briefly for proper init
  SPI_OFF_SD;
  delay(10);
  
  if (!SD.begin(SD_CS, spi))
  {
    Serial.println("SD Card initialization failed!");
    SPI_OFF_SD;
    SPI_ON_TFT;
    tft.fillScreen(TFT_RED);
    tft.setCursor(10, 150);
    tft.println("SD Card Error!");
    SPI_OFF_TFT;
    while (1)
    {
      delay(1000);
    }
  }

  Serial.println("SD Card initialized successfully");
  
  // Disable SD, enable TFT for display
  SPI_OFF_SD;
  SPI_ON_TFT;
  
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 150);
  tft.println("SD Card OK!");
  delay(1000);

  // Scan for JPG files
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 150);
  tft.println("Scanning for images...");
  
  SPI_OFF_TFT;

  scanSDCard();

  if (imageCount == 0)
  {
    Serial.println("No JPG images found on SD card!");
    SPI_ON_TFT;
    tft.fillScreen(TFT_YELLOW);
    tft.setCursor(10, 150);
    tft.println("No JPG files found!");
    SPI_OFF_TFT;
    while (1)
    {
      delay(1000);
    }
  }

  Serial.print("Found ");
  Serial.print(imageCount);
  Serial.println(" images");

  SPI_ON_TFT;
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 150);
  tft.print("Found ");
  tft.print(imageCount);
  tft.println(" images!");
  delay(2000);

  tft.fillScreen(TFT_BLACK);
  SPI_OFF_TFT;
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
  // Enable SD card for file operations
  SPI_ON_SD;
  
  File root = SD.open("/");
  if (!root)
  {
    Serial.println("Failed to open root directory");
    SPI_OFF_SD;
    return;
  }

  imageCount = 0;

  File file = root.openNextFile();
  while (file && imageCount < 100)
  {
    if (!file.isDirectory())
    {
      String fileName = String(file.name());
      
      // Skip hidden files (starting with .)
      if (fileName.startsWith(".") || fileName.startsWith("/."))
      {
        file.close();
        file = root.openNextFile();
        continue;
      }
      
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
  
  // Disable SD after operations
  SPI_OFF_SD;
}

void displayImage(String filename)
{
  Serial.println("====================");
  Serial.print("Displaying: ");
  Serial.println(filename);
  
  uint32_t freeHeap = ESP.getFreeHeap();
  Serial.print("Free heap: ");
  Serial.print(freeHeap);
  Serial.println(" bytes");

  // Clear screen - enable TFT
  SPI_OFF_SD;
  SPI_ON_TFT;
  tft.fillScreen(TFT_BLACK);
  SPI_OFF_TFT;

  // Draw JPEG from SD card directly to screen
  // TJpg_Decoder will call our tft_output callback which manages CS pins
  // Enable SD for file reading
  SPI_ON_SD;
  uint32_t t = millis();
  TJpgDec.drawSdJpg(0, 0, ("/" + filename).c_str());
  t = millis() - t;
  SPI_OFF_SD;

  Serial.print("Time taken: ");
  Serial.print(t);
  Serial.println(" ms");
  
  freeHeap = ESP.getFreeHeap();
  Serial.print("Free heap after: ");
  Serial.print(freeHeap);
  Serial.println(" bytes");
  Serial.println("Image displayed successfully!");
  Serial.println("====================");
}
