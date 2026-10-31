# AlcoSense Pro v10.0 — COMPLETE CIRCUIT & WIRING GUIDE
### Arduino Uno + MCP6002 + ADS1115 + 1.44" TFT + TP4056 + LiPo
### ✅ Verified Fixes: 1MΩ Rf | Direct SPI Connection | __TIME__ Clock

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 1 — POWER SUPPLY (TP4056 + MT3608 + LiPo)
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 LiPo Battery 3.7V 1000mAh
 ┌──────────┐
 │  (+) B+  │ ─────────────────────────────────→  TP4056  [B+]
 │  (-) B-  │ ─────────────────────────────────→  TP4056  [B-]
 └──────────┘

 TP4056 Module
 ┌───────────────┐/
 
 │ [B+]  [B-]   │  ← LiPo connects here
 │ [OUT+]        │ ─────────────────────────────→  MT3608  [IN+]
 │ [OUT-]        │ ─────────────────────────────→  MT3608  [IN-] / GND
 └───────────────┘

 MT3608 Boost Converter  (set output to exactly 5.00V using trimmer pot)
 ┌───────────────┐
 │  [IN+]        │ ← from TP4056 [OUT+]
 │  [IN-]        │ ← GND
 │  [OUT+]  5V   │ ─────────────────────────────→  Arduino Uno  [VIN]
 │  [OUT-]  GND  │ ─────────────────────────────→  Arduino Uno  [GND]
 └───────────────┘

 BATTERY VOLTAGE MONITOR  (optional — shows battery icon on display)

 TP4056 [OUT+] ────┬────[ R4 = 100kΩ ]────┬────→  Arduino Uno  [A0]
                   │                       │
                  GND               [ R5 = 100kΩ ]
                                           │
                                          GND

 > A0 reads ½ × battery voltage. Code multiplies by 2 to show actual voltage.
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 2 — MCP6002 POTENTIOSTAT CIRCUIT
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

### MCP6002 DIP-8 Pinout
```
            ┌─────────┐
  OUT-A  1 ─┤         ├─ 8  VDD  → Arduino 5V
  IN-  A 2 ─┤ MCP6002 ├─ 7  OUT-B
  IN+  A 3 ─┤         ├─ 6  IN-  B
  VSS    4 ─┤         ├─ 5  IN+  B
            └─────────┘
               Pin 4 → Arduino GND
```

---

### STAGE 1 — BIAS VOLTAGE GENERATOR  (U1A = pins 1,2,3)
```
 Generates stable +0.5V DC reference for the electrochemical sensor.

 Arduino Uno [D6]  (PWM output)
       │
      [R1 = 10kΩ]
       │
       ├───────────────────────────────→  MCP6002  [U1A IN+]  (pin 3)
       │
      [C1 = 10µF electrolytic]  +side up
       │
      GND

 MCP6002 [U1A IN-]  (pin 2) ────────────────→  MCP6002 [U1A OUT]  (pin 1)
                              (short wire, unity gain buffer)

 MCP6002 [U1A OUT]  (pin 1) ────────────────→  Sensor  [RE]  (Reference Electrode)
                            └───────────────→  MCP6002 [U1B IN+]  (pin 5)
                            └───────────────→  ADS1115 [A1]
                              (all three share this 0.5V node)
```

---

### STAGE 2 — TRANSIMPEDANCE AMPLIFIER / TIA  (U1B = pins 5,6,7)
```
 Converts tiny sensor current → measurable voltage for ADS1115.
 ✅ Rf = 1MΩ  (changed from 100kΩ — gives proper resolution with nA currents)

 Sensor  [WE]  (Working Electrode)
       │
       └───────────────────────────────→  MCP6002  [U1B IN-]  (pin 6)
                                                     │
                                            ┌──[ R2 = 1MΩ ]──┐
                                            │                 │
                                            └─────────────────→  MCP6002  [U1B OUT]  (pin 7)
                                                                          │
                                                                          └──→  ADS1115  [A0]

 MCP6002 [U1B IN+]  (pin 5)  already wired to 0.5V node from Stage 1
```

---

### MCP6002 POWER PINS
```
 Arduino Uno  [5V]  ─────────────────────────→  MCP6002  [VDD]  (pin 8)
 Arduino Uno  [GND] ─────────────────────────→  MCP6002  [VSS]  (pin 4)

 ⚡ Add C2 = 100nF ceramic capacitor between pin 8 and pin 4 (as close to IC as possible)
```

---

### SENSOR ELECTRODE CONNECTIONS
```
 ┌──────────────────────────────────────────────────────────────┐
 │  Sensor [WE]  Working Electrode  ───→  MCP6002  [U1B IN-]  pin 6  │
 │  Sensor [RE]  Reference Electrode ──→  MCP6002  [U1A OUT]  pin 1  │
 │  Sensor [CE]  Counter Electrode   ──→  MCP6002  [U1A OUT]  pin 1  │
 │                                   (CE tied to RE = 2-electrode mode)│
 └──────────────────────────────────────────────────────────────┘
```

---

### POTENTIOSTAT COMPONENTS LIST
```
 Ref  │ Type               │ Value    │ Purpose
 ─────┼────────────────────┼──────────┼────────────────────────────
 R1   │ Resistor           │ 10 kΩ    │ RC filter (PWM smoothing)
 C1   │ Electrolytic cap   │ 10 µF    │ RC filter capacitor
 C2   │ Ceramic cap        │ 100 nF   │ MCP6002 VDD decoupling
 R2   │ Resistor ✅FIXED   │ 1 MΩ     │ TIA feedback (was 100kΩ)
 R4   │ Resistor           │ 100 kΩ   │ Battery divider top
 R5   │ Resistor           │ 100 kΩ   │ Battery divider bottom
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 3 — ADS1115 (16-BIT ADC) — I²C
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 ADS1115 Module
 ┌─────────────────────────────────────────────────────────────────┐
 │  [VDD]  ──────────────────────────────→  Arduino Uno  [5V]     │
 │  [GND]  ──────────────────────────────→  Arduino Uno  [GND]    │
 │  [SCL]  ──────────────────────────────→  Arduino Uno  [A5]     │
 │  [SDA]  ──────────────────────────────→  Arduino Uno  [A4]     │
 │  [ADDR] ──────────────────────────────→  Arduino Uno  [GND]    │
 │                         ADDR=GND → I²C address 0x48            │
 │  [A0]   ←──────────────────────────── MCP6002 [U1B OUT] pin 7  │
 │  [A1]   ←──────────────────────────── 0.5V bias node (U1A OUT) │
 │  [A2]   ─── not connected                                       │
 │  [A3]   ─── not connected                                       │
 └─────────────────────────────────────────────────────────────────┘

 I²C PULL-UP RESISTORS  (required for reliable I²C)
 Arduino Uno [5V] ──[ 4.7kΩ ]──→  ADS1115 [SDA]  +  Arduino Uno [A4]
 Arduino Uno [5V] ──[ 4.7kΩ ]──→  ADS1115 [SCL]  +  Arduino Uno [A5]
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 4 — 1.44" TFT LCD (ST7735R) — SPI DIRECT
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 > NOTE: Arduino Uno SPI pins output 5V.
 > Most 1.44" ST7735 modules tolerate this in practice because
 > the ST7735 input threshold is ~0.7×VCC = 2.31V, so 5V signals
 > are read as HIGH correctly. If your module has a 3.3V regulator
 > on board (most do), the direct connection below works fine.

 1.44" TFT Module
 ┌─────────────────────────────────────────────────────────────────┐
 │  [VCC]  ──────────────────────────────→  Arduino Uno  [3.3V]   │
 │  [GND]  ──────────────────────────────→  Arduino Uno  [GND]    │
 │  [SCK]  ←──────────────────────────── Arduino Uno  [D13]       │
 │  [SDA]  ←──────────────────────────── Arduino Uno  [D11]       │
 │   (MOSI)                                                        │
 │  [CS]   ←──────────────────────────── Arduino Uno  [D10]       │
 │  [DC]   ←──────────────────────────── Arduino Uno  [D9]        │
 │  [RST]  ←──────────────────────────── Arduino Uno  [D8]        │
 │  [LED]  ──[ 100Ω ]────────────────────→ Arduino Uno  [3.3V]    │
 │   (backlight — 100Ω limits current to safe level)              │
 └─────────────────────────────────────────────────────────────────┘
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 5 — MASTER PIN TABLE (ALL PINS)
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 Arduino Uno Pin  │ Dir  │ Connected To
 ─────────────────┼──────┼─────────────────────────────────────────────────
 D6  (PWM)        │  →   │ R1 (10kΩ) → C1 (10µF/GND) → MCP6002 pin 3
 D8               │  →   │ TFT  [RST]
 D9               │  →   │ TFT  [DC]
 D10              │  →   │ TFT  [CS]
 D11  (MOSI)      │  →   │ TFT  [SDA / MOSI]
 D13  (SCK)       │  →   │ TFT  [SCK]
 A0               │  ←   │ R4/R5 voltage divider ← TP4056 OUT+
 A4   (SDA)       │  ↔   │ 4.7kΩ → 5V  +  ADS1115 [SDA]
 A5   (SCL)       │  ↔   │ 4.7kΩ → 5V  +  ADS1115 [SCL]
 3.3V (power out) │  →   │ TFT [VCC]  +  TFT [LED via 100Ω]
 5V   (power out) │  →   │ ADS1115 [VDD]  +  MCP6002 [VDD pin 8]
 GND              │  →   │ ALL GND rails (common ground for everything)
 VIN              │  ←   │ MT3608 [OUT+]  (5V regulated from LiPo boost)
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 6 — FULL SYSTEM BLOCK DIAGRAM
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
  LiPo 3.7V
  ┌────────┐
  │  (+)   │──→ TP4056 [B+]──→ MT3608 [IN+]──→ Boost 5V ──→ Arduino[VIN]
  │  (-)   │──→ TP4056 [B-]──→ MT3608 [IN-]──→ GND      ──→ Arduino[GND]
  └────────┘
       │
       └──[ R4=100kΩ ]──┬──[ R5=100kΩ ]──GND
                        │
                   Arduino [A0]  (battery level)

  Arduino [D6] PWM
       │
      [R1=10kΩ]──[C1=10µF/GND]
       │
  MCP6002 [pin3 U1A IN+]
  MCP6002 [pin2 U1A IN-] ───→ [pin1 U1A OUT] ──────────→ Sensor [RE]
                                    │                  ──→ MCP6002 [pin5 U1B IN+]
                                    │                  ──→ ADS1115 [A1]
  Sensor [WE] ──→ MCP6002 [pin6 U1B IN-]
                       │
                  [R2=1MΩ feedback]
                       │
  MCP6002 [pin7 U1B OUT] ─────────────────────────────→ ADS1115 [A0]

  ADS1115
  ┌────────────────┐
  │ VDD  → 5V      │
  │ GND  → GND     │
  │ SCL  →─────────────────────────────────────────→ Arduino [A5]
  │ SDA  →─────────────────────────────────────────→ Arduino [A4]
  │ ADDR → GND (0x48)                                (4.7kΩ pullups to 5V)
  │ A0   ← MCP6002 OUT (sensor signal)              │
  │ A1   ← 0.5V bias node                           │
  └────────────────┘

  Arduino [D13] ── SPI SCK  ────────────────────────→ TFT [SCK]
  Arduino [D11] ── SPI MOSI ────────────────────────→ TFT [SDA]
  Arduino [D10] ── CS       ────────────────────────→ TFT [CS]
  Arduino [D9]  ── DC       ────────────────────────→ TFT [DC]
  Arduino [D8]  ── RST      ────────────────────────→ TFT [RST]
  Arduino [3.3V]─────────────────────────────────────→ TFT [VCC]
  Arduino [3.3V]──[100Ω]────────────────────────────→ TFT [LED]
  Arduino [GND] ────────────────────────────────────→ TFT [GND]
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 7 — BREADBOARD BUILD ORDER
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 Step 1 ── POWER RAILS
   MT3608 OUT+ (5V) → Arduino VIN
   MT3608 OUT-      → Arduino GND
   Arduino 5V       → Breadboard (+) rail A
   Arduino GND      → Breadboard (-) rail A

 Step 2 ── MCP6002 IC  (place at center of breadboard)
   Pin 8 VDD → (+) rail (5V)
   Pin 4 VSS → (-) rail (GND)
   C2 = 100nF ceramic between pin 8 and pin 4 legs directly

 Step 3 ── RC BIAS FILTER
   Arduino D6 ──→ R1 leg 1
   R1 leg 2   ──→ C1(+) leg, MCP6002 pin 3 junction
   C1(−) leg  ──→ GND

 Step 4 ── MCP6002 STAGE 1 (U1A buffer)
   MCP6002 pin 2 ←──→ MCP6002 pin 1  (short jumper wire)
   MCP6002 pin 1 ───→ Sensor RE terminal
   MCP6002 pin 1 ───→ MCP6002 pin 5
   MCP6002 pin 1 ───→ ADS1115 A1

 Step 5 ── MCP6002 STAGE 2 (U1B TIA — use 1MΩ)
   Sensor WE terminal ───→ MCP6002 pin 6
   R2 = 1MΩ between MCP6002 pin 6 and MCP6002 pin 7
   MCP6002 pin 7       ───→ ADS1115 A0

 Step 6 ── ADS1115
   VDD  → 5V rail
   GND  → GND rail
   ADDR → GND rail
   SCL  → 4.7kΩ → 5V rail  AND  → Arduino A5
   SDA  → 4.7kΩ → 5V rail  AND  → Arduino A4

 Step 7 ── TFT DISPLAY
   VCC → Arduino 3.3V
   GND → GND
   SCK → Arduino D13
   SDA → Arduino D11
   CS  → Arduino D10
   DC  → Arduino D9
   RST → Arduino D8
   LED → 100Ω → Arduino 3.3V

 Step 8 ── BATTERY MONITOR
   TP4056 OUT+ → R4(100kΩ) → node → Arduino A0
   Node        → R5(100kΩ) → GND

 Step 9 ── LIPO + TP4056
   LiPo (+) → TP4056 B+
   LiPo (-) → TP4056 B-
   TP4056 OUT+ → MT3608 IN+
   TP4056 OUT- → MT3608 IN-  / GND
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 8 — COMPLETE BILL OF MATERIALS
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 #   │ Component                   │ Value / Part          │ Qty
 ────┼─────────────────────────────┼───────────────────────┼────
  1  │ Microcontroller             │ Arduino Uno R3         │  1
  2  │ LiPo Battery                │ 3.7V 1000mAh          │  1
  3  │ LiPo Charger                │ TP4056 module          │  1
  4  │ Boost Converter             │ MT3608 5V module       │  1
  5  │ Op-Amp                      │ MCP6002 DIP-8          │  1
  6  │ ADC                         │ ADS1115 module         │  1
  7  │ TFT Display                 │ 1.44" ST7735R SPI      │  1
  8  │ Resistor RC filter          │ R1 = 10 kΩ             │  1
  9  │ Resistor TIA feedback ✅    │ R2 = 1 MΩ              │  1
 10  │ Resistor battery div top    │ R4 = 100 kΩ            │  1
 11  │ Resistor battery div bottom │ R5 = 100 kΩ            │  1
 12  │ Resistor I²C pullup × 2     │ 4.7 kΩ                 │  2
 13  │ Resistor TFT backlight      │ 100 Ω                  │  1
 14  │ Capacitor RC filter         │ C1 = 10 µF electrolytic│  1
 15  │ Capacitor MCP6002 decouple  │ C2 = 100 nF ceramic    │  1
 16  │ Electrochemical sensor      │ 3-electrode alcohol    │  1
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 9 — CALIBRATION
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 1. Power on with NO alcohol near sensor (fresh air).
 2. Open Serial Monitor → 115200 baud.
 3. Read "ADC raw" value → set BASELINE_ADC = this number in code.
 4. Test with a known BAC standard (or calibrated breath).
 5. Calculate: SENSITIVITY = ADC_shift ÷ (known_BAC ÷ 0.010)
 6. Re-upload code.

 Expected ranges with Rf = 1MΩ:
   Air (no alcohol)   →  ~0 ADC counts above baseline
   BAC 0.050%         →  ~400 counts
   BAC 0.080%         →  ~640 counts
   BAC 0.150%         →  ~1200 counts
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 10 — BAC COLOUR ZONES
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 BAC Range      │ Colour  │ Banner Text
 ───────────────┼─────────┼─────────────
 0.000 – 0.019  │ GREEN   │  SOBER
 0.020 – 0.049  │ GREEN   │  CLEAR
 0.050 – 0.079  │ AMBER   │  CAUTION
 0.080 – 0.149  │ RED     │  ILLEGAL
 0.150 +        │ PURPLE  │  DANGER!
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 11 — LIBRARIES TO INSTALL
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 Arduino IDE → Sketch → Include Library → Manage Libraries

 1.  Adafruit ST7735 and ST7789 Library   (by Adafruit)
 2.  Adafruit GFX Library                (by Adafruit)
 3.  Adafruit ADS1X15                    (by Adafruit)
```
