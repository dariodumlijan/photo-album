// ====== UX CONFIGURATION ======
// Multi-tap configurations for image delay: {taps, delay, label}
struct ImageDelayConfig
{
  int taps;
  unsigned long delay;
  const char *label;
};

ImageDelayConfig delay_configs[] = {
    {1, 5000, "5 sec"},   // 1 tap: 5 seconds
    {2, 30000, "30 sec"}, // 2 taps: 30 seconds
    {3, 60000, "1 min"},  // 3 taps: 1 minute
    {4, 300000, "5 min"}, // 4 taps: 5 minutes
    {5, 0, "Infinite"}    // 5 taps: manual mode (infinite delay)
};
#define NUM_DELAY_CONFIGS (sizeof(delay_configs) / sizeof(delay_configs[0]))

// Button and touch behavior settings - easily adjustable by developers
#define BUTTON_DEBOUNCE 300       // Button debounce time in milliseconds
#define TOUCH_DEBOUNCE 300        // Touch debounce time
#define MULTI_TAP_WINDOW 1000     // Time window for detecting multiple taps (milliseconds)
#define MESSAGE_DISPLAY_TIME 3000 // Time to show on-screen messages (3 seconds)

// Center screen area for multi-tap detection (x-coordinates)
#define CENTER_TOUCH_LEFT 160  // Left boundary of center area (160px from left)
#define CENTER_TOUCH_RIGHT 320 // Right boundary of center area (320px from left)

// Default image lifetime (will be dynamically updated by center taps)
unsigned long IMAGE_LIFETIME = delay_configs[0].delay;

#include <Arduino.h>
#include <SPI.h>
#include "SD.h"
#include "FS.h"
#include <TJpg_Decoder.h>
#include <vector>

#include <TFT_eSPI.h> // Hardware-specific library with built-in touch support

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// ====== HARDWARE CONFIGURATION ======
#define BOOT_BUTTON 0 // GPIO0 is the boot button
#define TFT_BL 27     // Backlight control pin
#define SD_CS 5       // SD Card chip select pin
#define VSPI_MISO 19  // SD Card - VSPI pin
#define VSPI_MOSI 23  // SD Card - VSPI pin
#define VSPI_SCK 18   // SD Card - VSPI pin

// SPI control macros
#define SPI_ON_SD digitalWrite(SD_CS, LOW)
#define SPI_OFF_SD digitalWrite(SD_CS, HIGH)

// Global variables
std::vector<String> file_list;
int file_index = 0;
bool display_on = true;
unsigned long last_button_press = 0;

// Touch multi-tap tracking variables
int taps = 0;
unsigned long last_tap_time = 0;
unsigned long last_touch_time = 0;

// Message display tracking
bool showing_message = false;
unsigned long message_start_time = 0;
String current_message = "";

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

// Function to display a centered message on screen for a specific duration
void showMessage(String message)
{
  showing_message = true;
  message_start_time = millis();
  current_message = message;

  // Show message
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  // Calculate text dimensions
  int16_t text_width = message.length() * 12; // Approximate character width
  int16_t text_height = 16;                   // Approximate text height for size 2
  int16_t padding = 8;                        // Padding above and below text

  // Position text at top center
  int16_t x_pos = (tft.width() - text_width) / 2;
  int16_t y_pos = padding; // Position at top with padding

  // Draw black bar spanning full width with padding
  int16_t bar_height = text_height + (2 * padding);
  tft.fillRect(0, 0, tft.width(), bar_height, TFT_BLACK);

  tft.setCursor(x_pos, y_pos);
  tft.print(message);
}

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
  // Simple boot button - just toggle display on/off
  if (digitalRead(BOOT_BUTTON) == LOW &&
      (millis() - last_button_press > BUTTON_DEBOUNCE))
  {
    display_on = !display_on; // Toggle display state

    if (display_on)
    {
      digitalWrite(TFT_BL, HIGH); // Turn backlight on
      tft.fillScreen(TFT_BLACK);  // Clear any artifacts
      flag = true;                // Immediately show current image
    }
    else
    {
      tft.fillScreen(TFT_BLACK); // Fill screen black before turning off
      digitalWrite(TFT_BL, LOW); // Turn backlight off
    }

    last_button_press = millis();

    // Wait for button release with debounce
    while (digitalRead(BOOT_BUTTON) == LOW)
    {
      delay(50);
    }
    delay(BUTTON_DEBOUNCE);
  }

  // Check if we need to clear a message that has been displayed long enough
  if (showing_message && (millis() - message_start_time >= MESSAGE_DISPLAY_TIME))
  {
    showing_message = false;
    tft.fillScreen(TFT_BLACK); // Clear message
    flag = true;               // Force image refresh
  }

  // Auto-advance images every x seconds or when flag is set (only when display is on and not showing message)
  // Skip auto-advance if IMAGE_LIFETIME is 0 (manual mode) unless flag is set for manual navigation
  if (display_on && !showing_message && (((IMAGE_LIFETIME > 0) && (millis() - runtime >= IMAGE_LIFETIME)) || flag))
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

  // Process pending multi-taps after window expires
  if (taps > 0 && (millis() - last_tap_time >= MULTI_TAP_WINDOW))
  {
    // Set delay based on tap count and show message
    bool config_found = false;
    for (int i = 0; i < NUM_DELAY_CONFIGS; i++)
    {

      if (delay_configs[i].taps == taps)
      {
        IMAGE_LIFETIME = delay_configs[i].delay;
        showMessage("Photo's lifetime: " + String(delay_configs[i].label));
        config_found = true;
        break;
      }
    }

    // If tap count exceeds configured options, use the last (highest) config
    if (!config_found)
    {
      Serial.println("No config found, using default");
      IMAGE_LIFETIME = delay_configs[NUM_DELAY_CONFIGS - 1].delay;
      showMessage("Photo's lifetime: " + String(delay_configs[NUM_DELAY_CONFIGS - 1].label));
    }

    // Reset image timer to apply new delay
    runtime = millis();
    taps = 0; // Reset tap count after processing
  }

  // Check for touch input using TFT_eSPI's built-in touch support
  uint16_t touch_x = 0, touch_y = 0;

  // getTouch returns true if touch is detected, threshold filters light touches
  if (tft.getTouch(&touch_x, &touch_y, 600))
  {
    // Check if touch is in center area (160-320 px width) for delay control
    if (touch_x >= CENTER_TOUCH_LEFT && touch_x <= CENTER_TOUCH_RIGHT)
    {
      // Center tap for changing image delay - no debounce needed for multi-tap

      if (millis() - last_tap_time < MULTI_TAP_WINDOW && last_tap_time > 0)
      {
        taps++;
        Serial.print("Incremented taps to: ");
        Serial.println(taps);
      }
      else
      {
        taps = 1; // Start new tap sequence
        Serial.println("Starting new tap sequence");
      }

      last_tap_time = millis();

      // Wait for touch release
      while (tft.getTouch(&touch_x, &touch_y, 600))
      {
        delay(50);
      }

      // Short delay to prevent immediate re-trigger from same touch
      delay(50);
    }
    else if ((touch_x < CENTER_TOUCH_LEFT || touch_x > CENTER_TOUCH_RIGHT) &&
             (millis() - last_touch_time > TOUCH_DEBOUNCE))
    {
      // Side touches (left/right) need debounce to prevent accidental navigation
      last_touch_time = millis();

      if (touch_x < CENTER_TOUCH_LEFT)
      {
        // Left side = previous image
        file_index = (file_index + file_list.size() - 2) % file_list.size();
        flag = true;
      }
      else if (touch_x > CENTER_TOUCH_RIGHT)
      {
        // Right side = next image
        flag = true;
      }

      // Wait for touch release with debounce
      while (tft.getTouch(&touch_x, &touch_y, 600))
      {
        delay(50);
      }

      // Additional debounce after release for side touches only
      delay(TOUCH_DEBOUNCE);
    }
  }

  delay(50); // Small delay to prevent excessive polling
}