# AlcoSense Pro — Sensor Strip Connection, Training & Execution Guide

---

## PART 1 — SENSOR STRIP (Electrode) CONNECTIONS

### What is the 3-Electrode Strip?
```
 Your electrochemical alcohol sensor strip has 3 pads/terminals:

  ┌─────────────────────────────────────┐
  │   WE   │   RE   │   CE            │
  │Working │ Refer- │ Counter          │
  │Electrd │  ence  │ Electrode        │
  └─────────────────────────────────────┘
```

### How to Connect Strip to MCP6002

```
 Strip [WE] ──────────────→ MCP6002 pin 6  (U1B IN-)
 Strip [RE] ──────────────→ MCP6002 pin 1  (U1A OUT / 0.5V bias node)
 Strip [CE] ──────────────→ MCP6002 pin 1  (tie CE to RE = 2-electrode mode)
```

> **If your strip has only 2 pads (WE and RE only):**
> Connect WE → MCP6002 pin 6, RE → MCP6002 pin 1.
> CE is not needed for 2-electrode strips.

### Physical Connection Tips
```
 ● Use crocodile clips or strip connectors — do NOT solder directly on strip
 ● Keep leads SHORT (< 10 cm) — long wires pick up noise and affect readings
 ● The WE wire matters most — keep it twisted with the RE wire
 ● Never let WE and RE touch each other (short circuit = wrong reading)
```

---

## PART 2 — SAMPLE PREPARATION

### Reference Solution (for calibration = baseline = 0.00 on display)
```
 Use plain clean water (distilled preferred, tap water acceptable).
 ● Fill a small clean beaker or cup with ~20 mL water
 ● Dip the sensor strip fully covering all 3 electrode pads
 ● Wait 30 seconds for it to stabilize before pressing upload
```

### Alcohol Test Solutions (for training)
```
 ┌──────────────────────────────────────────────────────────┐
 │  Solution            │ How to prepare       │ Expected   │
 │──────────────────────│──────────────────────│────────────│
 │ 1% ethanol/water     │ 1 mL IPA + 99 mL H₂O│ Low index  │
 │ 5% ethanol/water     │ 5 mL IPA + 95 mL H₂O│ Medium     │
 │ 10% ethanol/water    │ 10 mL IPA + 90 mL H₂O│ High       │
 │ Mouthwash/hand gel   │ Use directly         │ Very high  │
 └──────────────────────────────────────────────────────────┘

 Use isopropyl alcohol (IPA) or ethanol from pharmacy.
 Do NOT use methanol — toxic!
```

---

## PART 3 — STEP BY STEP EXECUTION

### Step 1 — Power Up
```
 1. Connect Arduino to PC via USB (for first-time use)
    OR connect LiPo battery through TP4056 → MT3608 → Arduino VIN
 2. Open Arduino IDE → open AlcoSense_Pro_Uno.ino
 3. Select Board: Arduino Uno, correct COM port
 4. Click Upload
```

### Step 2 — Boot Sequence (what you should see)
```
 ① RED flash on TFT    → display hardware working
 ② GREEN flash         → display confirmed
 ③ BLUE flash          → SPI communication OK
 ④ "AUTO-CALIBRATING" screen appears
    → Keep sensor in WATER (reference) during this phase
    → Progress bar counts to 10 over ~8 seconds
 ⑤ Main display appears with INDEX 0.00
```

### Step 3 — Calibration (first time only)
```
 During the "AUTO-CALIBRATING" screen:
 ● Sensor strip MUST be in plain water (no alcohol)
 ● Hold still — movement causes noise in readings
 ● The baseline is auto-saved to memory (no manual entry needed)
 ● Watch Serial Monitor: you'll see 10 "Cal sample" readings
```

### Step 4 — Testing Alcohol Response
```
 1. After calibration, move sensor from water to 5% ethanol solution
 2. Watch display — INDEX value should RISE within 2-3 seconds
 3. Move back to water — INDEX should fall back toward 0.00
 4. Response speed depends on:
    - How well electrodes are wetted
    - Concentration of ethanol
    - Temperature (room temp = best)
```

### Step 5 — SENSITIVITY Tuning
```
 In the code, find: float SENSITIVITY = 3200.0;

 ● If display maxes at 100.00 too easily → INCREASE SENSITIVITY (try 6400)
 ● If display barely moves with strong alcohol → DECREASE SENSITIVITY (try 1600)
 ● Rule: SENSITIVITY = (ADC_delta at 0.080% BAC) / 8

 Check Serial Monitor for "Delta:" value:
   Example: 5% ethanol gives Delta = 1200
   Set as reference for 0.080% BAC → SENSITIVITY = 1200 / 8 = 150
```

---

## PART 4 — SERIAL MONITOR READINGS EXPLAINED

```
 Open Serial Monitor at 115200 baud. Each line shows:

 Smoothed:1847.2  Delta:23.4  mV:57.72  Index:2.61  BAC:0.0078%  Batt:4.10V
 │               │           │          │            │            │
 │               │           │          │            │            Battery voltage
 │               │           │          │            BAC in % (raw)
 │               │           │          Display value (0–100)
 │               │           Voltage at ADS A0 (mV)
 │               Change from baseline
 Moving-averaged ADC count
```

---

## PART 5 — DISPLAY ZONES / WHAT THE COLOURS MEAN

```
 Index Value  │ Colour  │ Banner   │ Equivalent BAC
 ─────────────┼─────────┼──────────┼───────────────
  0.00 –  6.67│ GREEN   │ SOBER    │ < 0.020%
  6.67 – 16.67│ GREEN   │ CLEAR    │ 0.020–0.050%
 16.67 – 26.67│ AMBER   │ CAUTION  │ 0.050–0.080%
 26.67 – 50.00│ RED     │ ILLEGAL  │ 0.080–0.150%
 50.00 – 100.0│ PURPLE  │ DANGER!  │ > 0.150%
```

---

## PART 6 — TROUBLESHOOTING

```
 Problem                │ Likely Cause           │ Fix
 ───────────────────────┼────────────────────────┼────────────────────────
 Display blank          │ Wrong TFT init type    │ Change TFT_INIT_TYPE
 INDEX always 0         │ No sensor current      │ Check WE/RE connections
 INDEX fluctuates wildly│ Long wires, bad contact│ Shorten + twist WE/RE
 INDEX goes negative    │ Reversed WE/RE         │ Swap WE and RE wires
 ADS1115 not found      │ I2C wiring error       │ Check SDA/SCL + pullups
 No change with alcohol │ Wrong Rf value         │ Confirm R2 = 1MΩ
```
