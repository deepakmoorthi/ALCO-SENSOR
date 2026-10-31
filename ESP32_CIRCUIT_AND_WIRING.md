# AlcoSense Pro v10.0 — ESP32 COMPLETE CIRCUIT & WIRING GUIDE
### ESP32 Dev Board + MCP6002 + ADS1115 + 1.44" TFT + TP4056 + LiPo

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 1 — POWER SUPPLY (TP4056 + MT3608 / Gold 6009 + LiPo)
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 LiPo Battery 3.7V 1000mAh
 ┌──────────┐
 │  (+) B+  │ ─────────────────────────────────→  TP4056  [B+]
 │  (-) B-  │ ─────────────────────────────────→  TP4056  [B-]
 └──────────┘

 TP4056 Module
 ┌───────────────┐
 │ [B+]  [B-]    │  ← LiPo connects here
 │ [OUT+]        │ ───────[ POWER SWITCH ]──────→  MT3608  [IN+]
 │ [OUT-]        │ ─────────────────────────────→  MT3608  [IN-] / GND
 └───────────────┘

 > **Power Switch Note**: Placing the ON/OFF slide/toggle switch here allows 
 > you to charge the battery (via TP4056 USB) even when the device is turned off.

 MT3608 or Gold 6009 Boost Converter  (set output to exactly 5.00V using trimmer pot)
 ┌───────────────┐
 │  [IN+]        │ ← from Power Switch
 │  [IN-]        │ ← GND
 │  [OUT+]  5V   │ ─────────────────────────────→  ESP32  [VIN / 5V]
 │  [OUT-]  GND  │ ─────────────────────────────→  ESP32  [GND]
 └───────────────┘

 BATTERY VOLTAGE MONITOR  (optional — shows battery icon on display)

 TP4056 [OUT+] ────┬────[ R4 = 10kΩ ]─────┬────→  ESP32  [GPIO 34]
                   │                      │
                  GND               [ R5 = 10kΩ ]
                                          │
                                         GND

 > GPIO 34 reads ½ × battery voltage. Max voltage is 4.2V, which becomes 2.1V.
 > This is safely below the ESP32's 3.3V ADC limit.
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 2 — MCP6002 POTENTIOSTAT CIRCUIT
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

### MCP6002 DIP-8 Pinout
```
            ┌─────────┐
  OUT-A  1 ─┤         ├─ 8  VDD  → ESP32 3.3V or 5V (from MT3608)
  IN-  A 2 ─┤ MCP6002 ├─ 7  OUT-B
  IN+  A 3 ─┤         ├─ 6  IN-  B
  VSS    4 ─┤         ├─ 5  IN+  B
            └─────────┘
               Pin 4 → Common GND
```

---

### STAGE 1 — BIAS VOLTAGE GENERATOR  (U1A = pins 1,2,3)
```
 Generates stable +0.5V DC reference for the electrochemical sensor.

 ESP32 [GPIO 27]  (PWM output generated at 3.3V logic)
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

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 3 — ADS1115 (16-BIT ADC) — I²C
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 ADS1115 Module
 ┌─────────────────────────────────────────────────────────────────┐
 │  [VDD]  ──────────────────────────────→  ESP32  [3.3V]           │
 │  [GND]  ──────────────────────────────→  ESP32  [GND]            │
 │  [SCL]  ──────────────────────────────→  ESP32  [GPIO 22]        │
 │  [SDA]  ──────────────────────────────→  ESP32  [GPIO 21]        │
 │  [ADDR] ──────────────────────────────→  ESP32  [GND]            │
 │                         ADDR=GND → I²C address 0x48             │
 │  [A0]   ←──────────────────────────── MCP6002 [U1B OUT] pin 7   │
 │  [A1]   ←──────────────────────────── 0.5V bias node (U1A OUT)  │
 │  [A2]   ─── not connected                                       │
 │  [A3]   ─── not connected                                       │
 └─────────────────────────────────────────────────────────────────┘

 (Note: The ESP32 provides internal pull-ups for I2C, but if communication
 is unstable, add external 4.7kΩ resistors from SDA/SCL to 3.3V)
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 4 — 1.44" TFT LCD (ST7735R) — SPI DIRECT
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 1.44" TFT Module (Using ESP32 Hardware VSPI pins for fast refresh)
 ┌─────────────────────────────────────────────────────────────────┐
 │  [VCC]  ──────────────────────────────→  ESP32  [3.3V]           │
 │  [GND]  ──────────────────────────────→  ESP32  [GND]            │
 │  [SCK]  ←──────────────────────────── ESP32  [GPIO 18]         │
 │  [SDA]  ←──────────────────────────── ESP32  [GPIO 23] (MOSI)  │
 │  [CS]   ←──────────────────────────── ESP32  [GPIO 5]          │
 │  [DC]   ←──────────────────────────── ESP32  [GPIO 2]          │
 │  [RST]  ←──────────────────────────── ESP32  [GPIO 4]          │
 │  [LED]  ──[ 100Ω ]────────────────────→ ESP32  [3.3V]            │
 │   (backlight — 100Ω limits current to safe level)               │
 └─────────────────────────────────────────────────────────────────┘
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 5 — MASTER PIN TABLE (ESP32)
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
 ESP32 Pin        │ Dir  │ Connected To
 ─────────────────┼──────┼─────────────────────────────────────────────────
 GPIO 27 (PWM)    │  →   │ R1 (10kΩ) → C1 (10µF/GND) → MCP6002 pin 3
 GPIO 4           │  →   │ TFT  [RST]
 GPIO 2           │  →   │ TFT  [DC]
 GPIO 5           │  →   │ TFT  [CS]
 GPIO 23 (MOSI)   │  →   │ TFT  [SDA / MOSI]
 GPIO 18 (SCK)    │  →   │ TFT  [SCK]
 GPIO 34 (ADC1)   │  ←   │ R4/R5 voltage divider ← TP4056 OUT+
 GPIO 21 (SDA)    │  ↔   │ ADS1115 [SDA]
 GPIO 22 (SCL)    │  ↔   │ ADS1115 [SCL]
 3.3V (power out) │  →   │ TFT [VCC], TFT [LED via 100Ω], ADS1115 [VDD]
 GND              │  →   │ ALL GND rails (common ground for everything)
 VIN / 5V         │  ←   │ MT3608 [OUT+] (5V regulated from LiPo boost)
```

---

## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
## SECTION 6 — FULL SYSTEM BLOCK DIAGRAM
## ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

```
  LiPo 3.7V
  ┌────────┐
  │  (+)   │──→ TP4056 [B+]──→ [POWER SW] ──→ MT3608 [IN+]──→ Boost 5V ──→ ESP32 [VIN/5V]
  │  (-)   │──→ TP4056 [B-]─────────────────→ MT3608 [IN-]──→ GND      ──→ ESP32 [GND]
  └────────┘
       │
       └──[ R4=10kΩ ]──┬──[ R5=10kΩ ]──GND
                       │
                  ESP32 [GPIO 34]  (battery level)

  ESP32 [GPIO 27] PWM
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
  │ VDD  → 3.3V    │
  │ GND  → GND     │
  │ SCL  →─────────────────────────────────────────→ ESP32 [GPIO 22]
  │ SDA  →─────────────────────────────────────────→ ESP32 [GPIO 21]
  │ ADDR → GND (0x48)                              │
  │ A0   ← MCP6002 OUT (sensor signal)             │
  │ A1   ← 0.5V bias node                          │
  └────────────────┘

  ESP32 [GPIO 18] ── SPI SCK  ──────────────────────→ TFT [SCK]
  ESP32 [GPIO 23] ── SPI MOSI ──────────────────────→ TFT [SDA]
  ESP32 [GPIO 5]  ── CS       ──────────────────────→ TFT [CS]
  ESP32 [GPIO 2]  ── DC       ──────────────────────→ TFT [DC]
  ESP32 [GPIO 4]  ── RST      ──────────────────────→ TFT [RST]
  ESP32 [3.3V]    ──────────────────────────────────→ TFT [VCC]
  ESP32 [3.3V]    ──[100Ω]──────────────────────────→ TFT [LED]
  ESP32 [GND]     ──────────────────────────────────→ TFT [GND]
```
