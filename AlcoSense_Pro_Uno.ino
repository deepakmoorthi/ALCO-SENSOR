// ================================================================
//  AlcoSense Pro v10.0  —  Arduino Uno
//  UPDATED: 0.00–100.00 scale | GAIN_FOUR | Moving average filter
// ================================================================
//  SCALE:   0.00  = no alcohol detected
//           100.00 = maximum detection limit (0.300% BAC)
//           e.g. legal limit 0.080% BAC → shows 26.67
//
//  SENSITIVITY: ADS1115 GAIN_FOUR (±1.024V, 0.03125 mV/bit)
//               Moving average over 5 samples = smooth but fast
//
//  CIRCUIT (no level shifter):
//   D6 (PWM) → RC(10kΩ+10µF) → MCP6002 pin3 (0.5V bias)
//   D8  → TFT [RST]      D9  → TFT [DC]
//   D10 → TFT [CS]       D11 → TFT [SDA/MOSI]
//   D13 → TFT [SCK]      A0  → Battery divider
//   A4  → ADS1115 [SDA]  A5  → ADS1115 [SCL]
//   ADS1115 A0 ← MCP6002 pin7 (TIA output)
//   ADS1115 A1 ← 0.5V bias node (MCP6002 pin1)
//
//  LIBRARIES (Manage Libraries):
//   ● Adafruit ST7735 and ST7789 Library
//   ● Adafruit GFX Library
//   ● Adafruit ADS1X15
// ================================================================

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// ──────────────────────────────────────────────────────────────
//  ★ TFT INIT — change if display blank
//    INITR_BLACKTAB       most 1.44" black-PCB modules
//    INITR_144GREENTAB    green-PCB modules
// ──────────────────────────────────────────────────────────────
#define TFT_INIT_TYPE   INITR_BLACKTAB

// ──────────────────────────────────────────────────────────────
//  PINS
// ──────────────────────────────────────────────────────────────
#define TFT_CS   10
#define TFT_DC    9
#define TFT_RST   8
#define BIAS_PIN  6
#define BATT_PIN A0

// ──────────────────────────────────────────────────────────────
//  OBJECTS
// ──────────────────────────────────────────────────────────────
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_ADS1115 ads;

// ──────────────────────────────────────────────────────────────
//  COLOURS  RGB565
// ──────────────────────────────────────────────────────────────
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_NAVY    0x000F
#define C_GREY    0x8410
#define C_LGREY   0xC618
#define C_STEEL   0x39E7
#define C_DKGREY  0x2104
#define C_GREEN   0x07E0
#define C_AMBER   0xFD20
#define C_RED     0xF800
#define C_PURPLE  0x780F
#define C_CYAN    0x07FF
#define C_YELLOW  0xFFE0

// ──────────────────────────────────────────────────────────────
//  SCALE SETTINGS
//  The 0–100 display maps 0.000% BAC → 0.00  and  0.300% BAC → 100.00
//  MAX_BAC_PCT is the physical BAC ceiling (0.300%)
//  DISPLAY_MAX = 100.00 (always)
// ──────────────────────────────────────────────────────────────
const float MAX_BAC_PCT  = 0.300;   // physical max BAC for calibration
const float DISPLAY_MAX  = 100.00;  // shown in display

// ── ADS1115 GAIN_FOUR: ±1.024V, LSB = 0.03125 mV/bit ─────────
const float LSB_MV  = 0.03125;     // mV per ADC bit at GAIN_FOUR

// ── CURRENT MEASUREMENT ────────────────────────────────────────
// The MCP6002 TIA converts sensor current → voltage: V = I × Rf
// We back-calculate:  I(nA) = ΔV(mV) / Rf(MΩ)
// Proof: I(A) = V(V)/R(Ω) → I(nA) = V(mV)/R(MΩ)  ✓
const float RF_MOHM = 1.0;         // Feedback resistor = 1 MΩ (R2)

// ── SENSITIVITY: nA per 0.010% BAC ─────────────────────────────
// This is the physically correct unit for an amperometric sensor.
// Typical enzyme electrodes: 50–500 nA per 0.010% BAC.
// Start at 50.0, tune up/down after testing a known sample.
float SENSITIVITY_NA = 50.0;       // nA per 0.010% BAC — TUNE ME

// Bias PWM: 0.35V = (0.35/5.0)×255 = 17.85 → 18
// ✅ 0.35V is optimal for Ferrocene mediator (E° ≈ +0.35V vs RE)
// Do NOT go above 0.40V — causes non-specific background oxidation
const uint8_t BIAS_PWM = 18;

// ──────────────────────────────────────────────────────────────
//  MOVING AVERAGE FILTER  (5 samples → smooth + fast)
// ──────────────────────────────────────────────────────────────
const int  MA_SIZE   = 5;
int16_t    maBuffer[MA_SIZE];
int        maIndex   = 0;
long       maSum     = 0;
bool       maFilled  = false;

float movingAverage(int16_t newVal) {
  maSum -= maBuffer[maIndex];
  maBuffer[maIndex] = newVal;
  maSum += newVal;
  maIndex = (maIndex + 1) % MA_SIZE;
  if (maIndex == 0) maFilled = true;
  int count = maFilled ? MA_SIZE : (maIndex == 0 ? MA_SIZE : maIndex);
  return (float)maSum / (float)count;
}

// ──────────────────────────────────────────────────────────────
//  AUTO-CALIBRATION
// ──────────────────────────────────────────────────────────────
float   baselineADC  = 0.0;
bool    calibrated   = false;
int     calSamples   = 0;
long    calSum       = 0;
const int CAL_COUNT  = 10;

// ──────────────────────────────────────────────────────────────
//  CLOCK  (auto-sets to upload time)
// ──────────────────────────────────────────────────────────────
uint8_t clk_hh = atoi(__TIME__);
uint8_t clk_mm = atoi(__TIME__ + 3);
uint8_t clk_ss = atoi(__TIME__ + 6);
uint8_t dt_d = 22, dt_m = 2, dt_y = 26;

// ──────────────────────────────────────────────────────────────
//  TIMING
// ──────────────────────────────────────────────────────────────
unsigned long prevClock = 0;
unsigned long prevRead  = 0;
const unsigned long CLOCK_MS = 1000UL;
const unsigned long READ_MS  = 500UL;   // read every 0.5s for fast response

// ──────────────────────────────────────────────────────────────
//  STATE
// ──────────────────────────────────────────────────────────────
float prevDisplay  = -1.0;
bool  firstDraw    = true;

// ──────────────────────────────────────────────────────────────
//  ZONE COLOUR  (based on 0–100 display scale)
//   0–16.67  = SOBER/CLEAR   → GREEN   (< 0.050% BAC)
//  16.67–26.67 = CAUTION     → AMBER   (0.050–0.080%)
//  26.67–50.00 = ILLEGAL     → RED     (0.080–0.150%)
//  50.00+    = DANGER        → PURPLE  (≥ 0.150%)
// ──────────────────────────────────────────────────────────────
uint16_t zoneColour(float d) {
  if (d < 16.67) return C_GREEN;
  if (d < 26.67) return C_AMBER;
  if (d < 50.00) return C_RED;
  return C_PURPLE;
}

const __FlashStringHelper* zoneText(float d) {
  if (d <  6.67) return F("   SOBER   ");
  if (d < 16.67) return F("   CLEAR   ");
  if (d < 26.67) return F("  CAUTION  ");
  if (d < 50.00) return F("  ILLEGAL  ");
  return                F("  DANGER!  ");
}

// ──────────────────────────────────────────────────────────────
//  TFT COLOUR FLASH (boot test — confirms TFT is alive)
// ──────────────────────────────────────────────────────────────
void tftColourTest() {
  tft.fillScreen(C_RED);   delay(400);
  tft.fillScreen(C_GREEN); delay(400);
  tft.fillScreen(0x001F);  delay(400); // blue
  tft.fillScreen(C_BLACK);
}

// ──────────────────────────────────────────────────────────────
//  DRAW: STATIC SKELETON
// ──────────────────────────────────────────────────────────────
void drawSkeleton() {
  tft.fillScreen(C_BLACK);

  // Header
  tft.fillRect(0, 0, 128, 18, C_NAVY);
  tft.setTextSize(1);
  tft.setTextColor(C_WHITE);
  tft.setCursor(4, 5);  tft.print(F("ALCOSENSE  PRO v10"));

  // Battery outline
  tft.drawRect(108, 2, 16, 9, C_LGREY);
  tft.fillRect(123, 4,  3, 5, C_LGREY);

  // Dividers
  tft.drawFastHLine(0, 18, 128, C_STEEL);

  // Time / Date labels
  tft.setTextColor(C_GREY);
  tft.setCursor(2, 21);  tft.print(F("TIME"));
  tft.setCursor(84, 21); tft.print(F("DATE"));
  tft.drawFastHLine(0, 41, 128, C_STEEL);

  // Scale label — shows 0–100 range
  tft.setTextColor(C_LGREY);
  tft.setCursor(28, 45); tft.print(F("INDEX  0-100"));

  // Progress bar track
  tft.drawRect( 6, 100, 116, 12, C_LGREY);
  tft.fillRect( 7, 101, 114,  10, C_DKGREY);

  // Scale marks under bar
  tft.setTextColor(C_GREY);
  tft.setCursor(  4, 115); tft.print(F("0"));
  tft.setCursor( 50, 115); tft.print(F("50"));
  tft.setCursor(108, 115); tft.print(F("100"));

  // Status banner outline
  tft.drawRect(4, 124, 120, 20, C_STEEL);

  // Raw debug rows
  tft.setTextColor(C_GREY);
  tft.setCursor(4, 147); tft.print(F("RAW:"));
  tft.setCursor(4, 156); tft.print(F(" nA:"));  // sensor current
}

// ──────────────────────────────────────────────────────────────
//  DRAW: CLOCK
// ──────────────────────────────────────────────────────────────
void drawClock() {
  char buf[9];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", clk_hh, clk_mm, clk_ss);
  tft.fillRect(0, 28, 76, 12, C_BLACK);
  tft.setTextColor(C_WHITE); tft.setTextSize(1);
  tft.setCursor(2, 30); tft.print(buf);

  snprintf(buf, sizeof(buf), "%02d/%02d/%02d", dt_d, dt_m, dt_y);
  tft.fillRect(76, 28, 52, 12, C_BLACK);
  tft.setCursor(78, 30); tft.print(buf);
}

// ──────────────────────────────────────────────────────────────
//  DRAW: MAIN VALUE  (0.00 – 100.00 scale)
// ──────────────────────────────────────────────────────────────
void drawValue(float disp) {
  uint16_t col = zoneColour(disp);

  // Large number — 6 chars wide "XXX.XX"
  char val[8];
  dtostrf(disp, 6, 2, val);

  tft.fillRect(2, 53, 124, 44, C_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(col);
  tft.setCursor(4, 55);
  tft.print(val);

  // Progress bar (0–100 maps to 0–114px)
  int16_t fw = (int16_t)((disp / DISPLAY_MAX) * 114.0);
  if (fw < 0)   fw = 0;
  if (fw > 114) fw = 114;
  tft.fillRect(7, 101, 114, 10, C_DKGREY);
  if (fw > 0) tft.fillRect(7, 101, fw, 10, col);

  // Colour band under status text reflects zone
  tft.fillRect(5, 125, 118, 18, col);
  tft.setTextSize(1);
  tft.setTextColor(C_BLACK);
  tft.setCursor(16, 130);
  tft.print(zoneText(disp));
}

// ──────────────────────────────────────────────────────────────
//  DRAW: RAW DEBUG ROWS (bottom)
// ──────────────────────────────────────────────────────────────
void drawDebug(int16_t raw, float mV) {
  tft.setTextSize(1);
  tft.setTextColor(C_CYAN);

  tft.fillRect(28, 147, 100, 8, C_BLACK);
  tft.setCursor(28, 147); tft.print(raw);

  tft.fillRect(28, 156, 100, 8, C_BLACK);
  tft.setCursor(28, 156); tft.print(mV, 3);
}

// ──────────────────────────────────────────────────────────────
//  DRAW: BATTERY
// ──────────────────────────────────────────────────────────────
void drawBattery(float v) {
  int pct = (int)((v - 3.0f) / 1.2f * 100.0f);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  uint16_t col = (pct > 20) ? C_GREEN : C_RED;
  int fw = (int)(13.0f * pct / 100.0f);
  tft.fillRect(109, 3, 13, 7, C_BLACK);
  if (fw > 0) tft.fillRect(109, 3, fw, 7, col);
  tft.drawRect(108, 2, 16, 9, C_LGREY);
  tft.fillRect(123, 4,  3, 5, C_LGREY);
}

// ──────────────────────────────────────────────────────────────
//  DRAW: CALIBRATION SCREEN
// ──────────────────────────────────────────────────────────────
void drawCalibrating(int done, int total) {
  tft.fillScreen(C_BLACK);
  tft.setTextColor(C_AMBER); tft.setTextSize(1);
  tft.setCursor(10, 18); tft.print(F("AUTO-CALIBRATING"));
  tft.setCursor(4, 34);  tft.print(F("Keep sensor in"));
  tft.setCursor(4, 46);  tft.print(F("REFERENCE SOLUTION"));
  tft.setCursor(4, 58);  tft.print(F("(no alcohol)"));

  // Progress bar
  tft.drawRect(6, 78, 116, 14, C_LGREY);
  int fw = (int)((float)done / total * 114.0f);
  tft.fillRect(7, 79, fw, 12, C_GREEN);
  tft.setTextColor(C_WHITE);
  tft.setCursor(48, 97); tft.print(done); tft.print(F("/")); tft.print(total);

  tft.setTextColor(C_GREY);
  tft.setCursor(4, 116); tft.print(F("Settling baseline..."));
}

// ══════════════════════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("\n=== AlcoSense Pro v10.0 — BOOT ==="));
  Serial.println(F("Scale: 0.00 = no alcohol, 100.00 = max detection"));

  // 1. Bias PWM (settle during init)
  pinMode(BIAS_PIN, OUTPUT);
  analogWrite(BIAS_PIN, BIAS_PWM);
  Serial.println(F("Bias 0.5V PWM started"));

  // 2. TFT Reset
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH); delay(5);
  digitalWrite(TFT_RST, LOW);  delay(20);
  digitalWrite(TFT_RST, HIGH); delay(200);

  // 3. TFT Init
  tft.initR(TFT_INIT_TYPE);
  tft.setRotation(0);
  tft.setSPISpeed(4000000);
  Serial.println(F("TFT init done — colour test running..."));
  tftColourTest();   // RED → GREEN → BLUE  confirms display alive

  // 4. ADS1115 Init — GAIN_FOUR for 4× sensitivity
  Wire.begin();
  if (!ads.begin()) {
    Serial.println(F("ERROR: ADS1115 not found!"));
    tft.fillScreen(C_BLACK);
    tft.setTextColor(C_RED);   tft.setTextSize(2);
    tft.setCursor(8, 40);  tft.print(F("ADS1115"));
    tft.setCursor(4, 62);  tft.print(F("NOT FOUND"));
    tft.setTextColor(C_LGREY); tft.setTextSize(1);
    tft.setCursor(4, 92);  tft.print(F("SDA->A4 SCL->A5"));
    tft.setCursor(4,104);  tft.print(F("ADDR->GND VDD->5V"));
    while (true) delay(1000);
  }
  // GAIN_FOUR: ±1.024V range, 0.03125 mV/bit — 4× more sensitive
  ads.setGain(GAIN_FOUR);
  ads.setDataRate(RATE_ADS1115_128SPS);  // 128 samples/sec = fast response
  Serial.println(F("ADS1115 OK — GAIN_FOUR (0.03125 mV/bit, ±1.024V)"));

  // 5. Auto-Calibration (10 samples, ~8 seconds)
  Serial.println(F("Auto-calibrating baseline — 10 samples..."));
  for (int i = 0; i < CAL_COUNT; i++) {
    drawCalibrating(i, CAL_COUNT);
    int16_t r = ads.readADC_Differential_0_1();
    calSum += r;
    calSamples++;
    // Pre-fill moving average buffer with baseline
    movingAverage(r);
    Serial.print(F("  Cal sample ")); Serial.print(i+1);
    Serial.print(F(": ")); Serial.println(r);
    delay(800);
  }
  baselineADC = (float)calSum / (float)calSamples;
  calibrated  = true;
  Serial.print(F("Baseline ADC = ")); Serial.println(baselineADC, 2);
  Serial.println(F("Calibration complete!"));
  Serial.println(F("\nFormat: SmoothedADC | Delta | mV | DisplayVal | BAC%"));

  // 6. Draw main GUI
  drawSkeleton();
  drawClock();
  drawValue(0.00);
  drawBattery(analogRead(BATT_PIN) * (5.0f / 1023.0f) * 2.0f);
}

// ══════════════════════════════════════════════════════════════
//  LOOP
// ══════════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  // Clock
  if (now - prevClock >= CLOCK_MS) {
    prevClock = now;
    clk_ss++;
    if (clk_ss >= 60) { clk_ss = 0; clk_mm++; }
    if (clk_mm >= 60) { clk_mm = 0; clk_hh++; }
    if (clk_hh >= 24)   clk_hh = 0;
    drawClock();
  }

  // Sensor read every 500ms for fast response
  if (now - prevRead >= READ_MS) {
    prevRead = now;

    // Raw differential read
    int16_t rawADC = ads.readADC_Differential_0_1();

    // Apply moving average (5 samples)
    float smoothed = movingAverage(rawADC);

    // Step 1 — Delta from calibrated baseline
    float delta = smoothed - baselineADC;

    // Step 2 — Convert ADC delta → differential voltage (mV)
    float delta_mV = delta * LSB_MV;

    // Step 3 — Back-calculate sensor current from TIA output
    //   TIA formula: Vout = Vbias - (I_sensor × Rf)
    //   → When I_sensor flows (oxidation), Vout DROPS below Vbias
    //   → delta_mV is NEGATIVE for real oxidation signal
    //   → Fix: negate delta_mV so current_nA is positive
    //   I(nA) = -ΔV(mV) / Rf(MΩ)   ← negative sign is correct!
    float current_nA = -delta_mV / RF_MOHM;
    if (current_nA < 0.0f) current_nA = 0.0f;  // clamp noise floor only

    // Step 4 — Convert sensor current → BAC %
    //   SENSITIVITY_NA = nA of current per 0.010% BAC
    float bac = (current_nA / SENSITIVITY_NA) * 0.010f;
    if (bac < 0.0f)    bac = 0.0f;

    // Step 5 — Scale to 0–100 display index
    float disp = (bac / MAX_BAC_PCT) * DISPLAY_MAX;
    if (disp < 0.0f)        disp = 0.0f;
    if (disp > DISPLAY_MAX) disp = DISPLAY_MAX;

    // Battery
    float battV = analogRead(BATT_PIN) * (5.0f / 1023.0f) * 2.0f;

    // Serial log — now shows physical current in nA
    Serial.print(F("Smooth:")); Serial.print(smoothed, 1);
    Serial.print(F("  dV_mV:")); Serial.print(delta_mV, 4);
    Serial.print(F("  I_nA:")); Serial.print(current_nA, 4);
    Serial.print(F("  Index:")); Serial.print(disp, 2);
    Serial.print(F("  BAC%:")); Serial.print(bac, 4);
    Serial.print(F("  Batt:")); Serial.print(battV, 2);
    Serial.println(F("V"));

    // Redraw when display changes by ≥ 0.01 on 0-100 scale
    if (firstDraw || fabsf(disp - prevDisplay) >= 0.01f) {
      drawValue(disp);
      prevDisplay = disp;
      firstDraw   = false;
    }

    drawDebug(rawADC, current_nA);  // show sensor current in nA
    drawBattery(battV);
  }
}
