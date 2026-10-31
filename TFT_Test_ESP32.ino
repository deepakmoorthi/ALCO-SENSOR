// ================================================================
//  TFT DISPLAY TESTER FOR ESP32
//  Tests: SPI wiring, RST/DC wiring, and Initialization
// ================================================================

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ──────────────────────────────────────────────────────────────
//  ESP32 PINS
// ──────────────────────────────────────────────────────────────
#define TFT_CS 5
#define TFT_DC 2
#define TFT_RST 4
// Make sure SCK is on 18 and MOSI (SDA) is on 23

// ──────────────────────────────────────────────────────────────
//  ★ TFT INIT — Change this if colors are weird or shifted!
//    INITR_BLACKTAB       most 1.44" black-PCB modules
//    INITR_144GREENTAB    green-PCB modules
// ──────────────────────────────────────────────────────────────
#define TFT_INIT_TYPE INITR_144GREENTAB

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- TFT TEST STARTING ---");

  // 1. Hardware Reset for TFT
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH); delay(10);
  digitalWrite(TFT_RST, LOW); delay(20);
  digitalWrite(TFT_RST, HIGH); delay(200);

  // 2. Initialize the display
  Serial.println("Initializing display...");
  tft.initR(TFT_INIT_TYPE);
  tft.setRotation(0);

  // 3. Color wipe test
  Serial.println("Filling RED...");
  tft.fillScreen(ST77XX_RED);
  delay(1000);
  
  Serial.println("Filling GREEN...");
  tft.fillScreen(ST77XX_GREEN);
  delay(1000);
  
  Serial.println("Filling BLUE...");
  tft.fillScreen(ST77XX_BLUE);
  delay(1000);

  // 4. Text test
  Serial.println("Drawing text...");
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 40);
  tft.print("TFT WORK!");
  
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(10, 70);
  tft.print("SPI is connected.");
}

void loop() {
  // Do nothing
}
