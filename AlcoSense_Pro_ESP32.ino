// ================================================================
//  AlcoSense Pro — ESP32 Version
//  (Reverted to classic loop logic without the state machine)
//  UPDATED: 0.00–100.00 scale | GAIN_FOUR | Moving average filter
// ================================================================

#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Wire.h>

// ──────────────────────────────────────────────────────────────
//  ★ TFT INIT — change if display blank
//    INITR_BLACKTAB       most 1.44" black-PCB modules
//    INITR_144GREENTAB    green-PCB modules (Use this to fix edge static lines)
// ──────────────────────────────────────────────────────────────
#define TFT_INIT_TYPE INITR_144GREENTAB

#define TFT_CS 5
#define TFT_DC 2
#define TFT_RST 4
#define BIAS_PIN 27
#define BATT_PIN 34

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_ADS1115 ads;

const float MAX_BAC_PCT = 0.300;
const float DISPLAY_MAX = 100.00;
const float LSB_MV = 0.03125;
const float RF_MOHM = 1.0; 

// Sensitivity calibration value. Adjust this if readings are too high/low.
// 476.0 is standard for 0.010% BAC
float SENSITIVITY_NA = 476.0; 

const uint8_t BIAS_PWM = 27;

const int MA_SIZE = 16;
int16_t maBuffer[MA_SIZE];
int maIndex = 0;
long maSum = 0;
bool maFilled = false;

float baselineADC = 0.0;
unsigned long prevRead = 0;
const unsigned long READ_MS = 500UL;

float movingAverage(int16_t newVal) {
  if (!maFilled && maIndex == 0) {
    for (int i=0; i<MA_SIZE; i++) maBuffer[i] = newVal;
    maSum = (long)newVal * MA_SIZE;
    maFilled = true;
  }
  maSum -= maBuffer[maIndex];
  maBuffer[maIndex] = newVal;
  maSum += newVal;
  maIndex = (maIndex + 1) % MA_SIZE;
  return (float)maSum / (float)MA_SIZE;
}

// Color definitions
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_NAVY    0x000F
#define C_GREY    0x8410
#define C_LGREY   0xC618
#define C_STEEL   0x39E7
#define C_DKGREY  0x2104
#define C_GREEN   0x07E0
#define C_AMBER   0xFD20
#define C_ORANGE  0xFD20
#define C_RED     0xF800
#define C_PURPLE  0x780F
#define C_MAGENTA 0xF81F

void drawSkeleton() {
  tft.fillScreen(C_BLACK);
  tft.fillRect(0, 0, 128, 18, C_NAVY);
  tft.setTextSize(1);
  tft.setTextColor(C_WHITE);
  tft.setCursor(4, 5);
  tft.print(F("ALCOSENSE PRO v10"));
  
  tft.drawFastHLine(0, 18, 128, C_STEEL);
  
  tft.setTextColor(C_GREY);
  tft.setCursor(2, 21);
  tft.print(F("TIME"));
  tft.setCursor(84, 21);
  tft.print(F("DATE"));
  
  tft.drawFastHLine(0, 41, 128, C_STEEL);
  
  tft.setTextColor(C_LGREY);
  tft.setCursor(28, 45);
  tft.print(F("BAC (0-100)"));
  
  // Progress bar outline
  tft.drawRect(6, 100, 116, 12, C_LGREY);
  tft.fillRect(7, 101, 114, 10, C_DKGREY);
  
  tft.setTextColor(C_GREY);
  tft.setCursor(4, 115);
  tft.print(F("0"));
  tft.setCursor(30, 115);
  tft.print(F(".05"));
  tft.setCursor(65, 115);
  tft.print(F(".08"));
  tft.setCursor(108, 115);
  tft.print(F(".15"));
}

void drawValue(float bac, float disp, float current_nA) {
  uint16_t col = C_GREEN;
  const char* status = "   SOBER   ";
  
  if (bac >= 0.150) { col = C_PURPLE; status = "  DANGER!  "; }
  else if (bac >= 0.080) { col = C_RED; status = "  ILLEGAL  "; }
  else if (bac >= 0.050) { col = C_AMBER; status = "  CAUTION  "; }
  else if (bac >= 0.020) { col = C_GREEN; status = "   CLEAR   "; }

  // Overwrite old value with blank box first
  tft.fillRect(0, 55, 128, 40, C_BLACK);
  
  tft.setTextSize(4);
  tft.setTextColor(col);
  
  char val[10];
  sprintf(val, "%6.2f", disp);
  tft.setCursor(5, 65);
  tft.print(val);

  // Progress Bar
  int16_t fw = (int16_t)((disp / DISPLAY_MAX) * 114.0);
  if (fw < 0) fw = 0;
  if (fw > 114) fw = 114;
  tft.fillRect(7, 101, 114, 10, C_DKGREY);
  if (fw > 0) tft.fillRect(7, 101, fw, 10, col);

  // Status Box
  tft.fillRect(5, 125, 118, 18, col);
  tft.setTextSize(1);
  tft.setTextColor(C_BLACK);
  tft.setCursor(30, 130);
  tft.print(status);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  const int pwmChannel = 0;
  ledcSetup(pwmChannel, 5000, 8);
  ledcAttachPin(BIAS_PIN, pwmChannel);
  ledcWrite(pwmChannel, BIAS_PWM);

  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH); delay(10);
  digitalWrite(TFT_RST, LOW); delay(20);
  digitalWrite(TFT_RST, HIGH); delay(200);

  tft.initR(TFT_INIT_TYPE);
  tft.setRotation(0);
  tft.fillScreen(C_BLACK);

  Wire.begin(); 
  if (!ads.begin()) {
    Serial.println(F("ADS1115 FAILED"));
    tft.setTextColor(C_RED);
    tft.setTextSize(2);
    tft.setCursor(10, 50);
    tft.print(F("ADS ERROR"));
    while (1) delay(100);
  }
  ads.setGain(GAIN_FOUR);
  ads.setDataRate(RATE_ADS1115_128SPS);

  tft.fillScreen(C_BLACK);
  tft.setTextColor(C_AMBER);
  tft.setTextSize(1);
  tft.setCursor(10, 18);
  tft.print(F("AUTO-CALIBRATING"));
  Serial.print(F("Calibrating... "));

  long calSum = 0;
  for (int i = 0; i < 10; i++) {
    calSum += ads.readADC_Differential_0_1();
    delay(500);
  }
  baselineADC = (float)calSum / 10.0f;
  Serial.println(F("Done!"));
  
  drawSkeleton();
}

void loop() {
  unsigned long now = millis();

  if (now - prevRead >= READ_MS) {
    prevRead = now;

    int16_t rawADC = ads.readADC_Differential_0_1();
    float smoothed = movingAverage(rawADC);
    float delta = smoothed - baselineADC;
    float delta_mV = delta * LSB_MV;
    float current_nA = -delta_mV / RF_MOHM;

    // Noise floor (Deadband)
    if (current_nA < 15.0f) current_nA = 0.0f;

    float bac = (current_nA / SENSITIVITY_NA) * 0.010f;
    if (bac < 0.0f) bac = 0.0f;
    
    float disp = (bac / MAX_BAC_PCT) * DISPLAY_MAX;
    if (disp > DISPLAY_MAX) disp = DISPLAY_MAX;

    drawValue(bac, disp, current_nA);
    
    float pinV = (analogRead(BATT_PIN) / 4095.0f) * 3.3f;
    float battV = pinV * 2.0f;

    // Matches old Serial Output format exactly
    Serial.print(F("Smooth:")); Serial.print(smoothed, 1);
    Serial.print(F("  dV_mV:")); Serial.print(delta_mV, 4);
    Serial.print(F("  I_nA:")); Serial.print(current_nA, 4);
    Serial.print(F("  Index:")); Serial.print(disp, 2);
    Serial.print(F("  BAC%:")); Serial.print(bac, 4);
    Serial.print(F("  Batt:")); Serial.print(battV, 2);
    Serial.println(F("V"));
  }
}
