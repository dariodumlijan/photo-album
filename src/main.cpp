// ====== UX CONFIGURATION ======
// Image delay configurations: {delay, label}
struct ImageDelayConfig
{
  unsigned long delay;
  const char *label;
};

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
    {0, "Infinite"} // manual mode (infinite delay)
};
#define NUM_DELAY_CONFIGS (sizeof(delay_configs) / sizeof(delay_configs[0]))

// Default image lifetime (will be dynamically updated by center taps)
unsigned long IMAGE_LIFETIME = delay_configs[0].delay;

// Button and touch behavior settings - easily adjustable by developers
#define BUTTON_DEBOUNCE 300   // Button debounce time in milliseconds
#define TOUCH_DEBOUNCE 150    // Touch debounce time
#define MULTI_TAP_WINDOW 1000 // Time window for detecting multiple taps (milliseconds)

// Center screen area for multi-tap detection (x-coordinates)
#define CENTER_TOUCH_LEFT 160  // Left boundary of center area (160px from left)
#define CENTER_TOUCH_RIGHT 320 // Right boundary of center area (320px from left)

#include <Arduino.h>
#include <SPI.h>
#include "SD.h"
#include "FS.h"
#include <TJpg_Decoder.h>
#include <vector>

#include <TFT_eSPI.h> // Hardware-specific library with built-in touch support

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// ====== HARDWARE CONFIGURATION ======
#define TOUCH_CALIBRATION {257, 3677, 223, 3571, 7} // calibrated using calibration/touch.cpp

#define BOOT_BUTTON 0 // GPIO0 is the boot button
#define TFT_BL 27     // Backlight control pin
#define SD_CS 5       // SD Card chip select pin
#define VSPI_MISO 19  // SD Card - VSPI pin
#define VSPI_MOSI 23  // SD Card - VSPI pin
#define VSPI_SCK 18   // SD Card - VSPI pin

// SPI control macros
#define SPI_ON_SD digitalWrite(SD_CS, LOW)
#define SPI_OFF_SD digitalWrite(SD_CS, HIGH)

// images
std::vector<String> file_list;
int file_index = 0;

// screen
bool display_on = true;
unsigned long button_pressed_at = 0;

// multi touch tracking
int taps = 0;
unsigned long tapped_at = 0;
unsigned long touched_at = 0;

// settings screen
bool settings_screen_visible = false;
int current_delay_index = 0; // Index into delay_configs array

// runtime variables
bool force_refresh = true;
unsigned long runtime = 0;

// Gets all image files in the SD card root directory
void get_pic_list(fs::FS &fs, const char *dirname, std::vector<String> &wavlist)
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

  // Return 1 to decode next block
  return 1;
}

// Function to display the settings screen
void showSettingsScreen()
{
  // Define UI color palette
  uint16_t button_bg = TFT_MAGENTA;
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);

  // Draw title
  String title = "Frame Interval";
  int16_t title_width = title.length() * 18; // Approximate character width
  int16_t title_x = (tft.width() - title_width) / 2 + 2;
  tft.setTextSize(3);
  tft.setCursor(title_x, 50);
  tft.print(title);

  // Draw current delay label in center with 20px margins
  String label = String(delay_configs[current_delay_index].label);
  int16_t label_width = label.length() * 18; // Approximate width for size 3
  int16_t label_x = (tft.width() - label_width) / 2;
  int16_t label_y = tft.height() / 2 - 12;
  tft.setTextSize(3);
  tft.setCursor(label_x, label_y);
  tft.print(label);

  // Draw +/- buttons
  tft.setTextSize(4);
  int16_t char_height = 28; // Approximate height at size 4
  int16_t char_width = 20;  // Approximate width at size 4
  int16_t button_size = 60;
  int16_t button_radius = 10;
  int16_t margin = 60;
  int16_t button_y = tft.height() / 2 - (button_size / 2);

  // - button (left side with margin)
  tft.fillRoundRect(margin, button_y, button_size, button_size, button_radius, button_bg);
  tft.setCursor(margin + (button_size - char_width) / 2, button_y + (button_size - char_height) / 2);
  tft.print("-");

  // + button (right side with margin)
  int16_t plus_x = tft.width() - margin - button_size;
  tft.fillRoundRect(plus_x, button_y, button_size, button_size, button_radius, button_bg);
  tft.setCursor(plus_x + (button_size - char_width) / 2, button_y + (button_size - char_height) / 2);
  tft.print("+");

  // Draw Save & Close button at bottom with extended width
  int16_t close_y = tft.height() - 80;
  int16_t close_button_width = 180; // Extended width to fit text
  int16_t close_x = (tft.width() - close_button_width) / 2;
  tft.fillRoundRect(close_x, close_y, close_button_width, 50, button_radius, button_bg);
  tft.setTextSize(2);

  // Center "Save & Close" text in the button
  String close_text = "Save & Close";
  int16_t close_text_width = close_text.length() * 12; // Approximate width for size 2
  tft.setCursor(close_x + (close_button_width - close_text_width) / 2, close_y + 18);
  tft.print(close_text);
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
  tft.setTextColor(TFT_WHITE);
  tft.setSwapBytes(true);

  // Display "Booting up..." message
  tft.setTextSize(4);
  String boot_title = "Booting up...";
  int16_t title_width = boot_title.length() * 24; // Approximate width for size 4
  int16_t title_x = (tft.width() - title_width) / 2;
  int16_t title_y = (tft.height() / 2) - 40;
  tft.setCursor(title_x, title_y);
  tft.print(boot_title);

  // Helper function to display setup steps
  auto displayStep = [&](const char *step)
  {
    tft.setTextSize(2);
    int16_t step_width = strlen(step) * 12; // Approximate width for size 2
    int16_t step_x = (tft.width() - step_width) / 2;
    int16_t step_y = title_y + 50;

    // Clear previous step text area
    tft.fillRect(0, step_y, tft.width(), 20, TFT_BLACK);

    tft.setCursor(step_x, step_y);
    tft.print(step);
  };

  // Calibrate touch for rotation 1 (landscape)
  uint16_t calData[5] = TOUCH_CALIBRATION;
  tft.setTouch(calData);

  Serial.println("TFT and Touch initialized");

  // Initialize TJpg_Decoder
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);

  // Initialize VSPI for SD Card (separate bus from TFT)
  SPI.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, SD_CS);

  // Initialize SD Card on VSPI
  displayStep("Mounting SD card...");
  SPI_ON_SD;
  if (!SD.begin(SD_CS, SPI, 4000000))
  {
    displayStep("SD Card Mount Failed!");
    Serial.println("SD Card Mount Failed!");
    SPI_OFF_SD;
    while (1)
      delay(1000);
  }
  Serial.println("SD Card Mount Succeeded");
  delay(300);

  // Get list of JPG files
  displayStep("Scanning SD card...");
  get_pic_list(SD, "/", file_list);
  delay(300);

  // Display photo count
  String photo_count = "Found " + String(file_list.size()) + " photos";
  displayStep(photo_count.c_str());
  delay(800);

  SPI_OFF_SD;

  Serial.println("Initialization complete!");

  // Clear screen before starting slideshow
  tft.fillScreen(TFT_BLACK);
}

void loop()
{
  // Simple boot button - just toggle display on/off
  if (digitalRead(BOOT_BUTTON) == LOW &&
      (millis() - button_pressed_at > BUTTON_DEBOUNCE))
  {
    display_on = !display_on; // Toggle display state

    if (display_on)
    {
      digitalWrite(TFT_BL, HIGH); // Turn backlight on
      tft.fillScreen(TFT_BLACK);  // Clear any artifacts
      force_refresh = true;       // Immediately show current image
    }
    else
    {
      tft.fillScreen(TFT_BLACK); // Fill screen black before turning off
      digitalWrite(TFT_BL, LOW); // Turn backlight off
    }

    button_pressed_at = millis();

    // Wait for button release with debounce
    while (digitalRead(BOOT_BUTTON) == LOW)
    {
      delay(50);
    }
    delay(BUTTON_DEBOUNCE);
  }

  // Auto-advance images every x seconds or when force_refresh is set (only when display is on and not showing settings)
  // Skip auto-advance if IMAGE_LIFETIME is 0 (manual mode) unless force_refresh is set for manual navigation
  if (display_on && !settings_screen_visible && (((IMAGE_LIFETIME > 0) && (millis() - runtime >= IMAGE_LIFETIME)) || force_refresh))
  {
    // Let TFT_eSPI manage CS internally
    tft.fillScreen(TFT_BLACK);

    // Add "/" prefix to filename for SD card path
    String filepath = "/" + file_list[file_index];

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

      if (result != 0)
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
    force_refresh = false;
  }

  // Process pending multi-taps after window expires - check for 2 taps to open settings
  if (taps == 2 && (millis() - tapped_at >= MULTI_TAP_WINDOW))
  {
    settings_screen_visible = true;
    showSettingsScreen();
    taps = 0; // Reset tap count after processing
  }
  else if (taps > 0 && (millis() - tapped_at >= MULTI_TAP_WINDOW))
  {
    // Reset tap count if not 2 taps
    taps = 0;
  }

  // Check for touch input using TFT_eSPI's built-in touch support
  uint16_t touch_x = 0, touch_y = 0;

  // getTouch returns true if touch is detected, threshold filters light touches
  if (tft.getTouch(&touch_x, &touch_y, 600))
  {
    if (settings_screen_visible)
    {
      // Handle settings screen touches
      int16_t button_size = 60;
      int16_t button_y = tft.height() / 2 - (button_size / 2);
      int16_t margin = 60;
      int16_t minus_x = margin;
      int16_t plus_x = tft.width() - margin - button_size;
      int16_t close_y = tft.height() - 80;
      int16_t close_button_width = 180;
      int16_t close_button_height = 50;
      int16_t close_x = (tft.width() - close_button_width) / 2;

      // Check for - button (left side)
      if (touch_x >= minus_x && touch_x <= (minus_x + button_size) &&
          touch_y >= button_y && touch_y <= (button_y + button_size))
      {
        // Decrease delay index
        current_delay_index--;
        if (current_delay_index < 0)
        {
          current_delay_index = NUM_DELAY_CONFIGS - 1; // Wrap to end
        }
        IMAGE_LIFETIME = delay_configs[current_delay_index].delay;
        showSettingsScreen();

        // Wait for touch release
        while (tft.getTouch(&touch_x, &touch_y, 600))
        {
          delay(50);
        }
        delay(200); // Debounce
      }
      // Check for + button (right side)
      else if (touch_x >= plus_x && touch_x <= (plus_x + button_size) &&
               touch_y >= button_y && touch_y <= (button_y + button_size))
      {
        // Increase delay index
        current_delay_index++;
        if (current_delay_index >= NUM_DELAY_CONFIGS)
        {
          current_delay_index = 0; // Wrap to start
        }
        IMAGE_LIFETIME = delay_configs[current_delay_index].delay;
        showSettingsScreen();

        // Wait for touch release
        while (tft.getTouch(&touch_x, &touch_y, 600))
        {
          delay(50);
        }
        delay(200); // Debounce
      }
      // Check for Close button
      else if (touch_x >= close_x && touch_x <= (close_x + close_button_width) &&
               touch_y >= close_y && touch_y <= (close_y + close_button_height))
      {
        // Close settings and return to slideshow
        settings_screen_visible = false;
        tft.fillScreen(TFT_BLACK);
        force_refresh = true;
        runtime = millis(); // Reset timer

        // Wait for touch release
        while (tft.getTouch(&touch_x, &touch_y, 600))
        {
          delay(50);
        }
        delay(200); // Debounce
      }
    }
    else
    {
      // Normal slideshow touch handling
      // Check if touch is in center area (160-320 px width) for delay control
      if (touch_x >= CENTER_TOUCH_LEFT && touch_x <= CENTER_TOUCH_RIGHT)
      {
        // Center tap for changing image delay - no debounce needed for multi-tap

        if (millis() - tapped_at < MULTI_TAP_WINDOW && tapped_at > 0)
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

        tapped_at = millis();

        // Wait for touch release
        while (tft.getTouch(&touch_x, &touch_y, 600))
        {
          delay(50);
        }

        // Short delay to prevent immediate re-trigger from same touch
        delay(50);
      }
      else if ((touch_x < CENTER_TOUCH_LEFT || touch_x > CENTER_TOUCH_RIGHT) &&
               (millis() - touched_at > TOUCH_DEBOUNCE))
      {
        // Side touches (left/right) need debounce to prevent accidental navigation
        touched_at = millis();

        if (touch_x < CENTER_TOUCH_LEFT)
        {
          // Left side = previous image
          file_index = (file_index + file_list.size() - 2) % file_list.size();
          force_refresh = true;
        }
        else if (touch_x > CENTER_TOUCH_RIGHT)
        {
          // Right side = next image
          force_refresh = true;
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
  }

  delay(50); // Small delay to prevent excessive polling
}