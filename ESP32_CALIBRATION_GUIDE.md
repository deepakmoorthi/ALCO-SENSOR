# AlcoSense Pro ESP32 v10.0 — User & Calibration Guide

This guide explains how to connect your electrochemical alcohol sensor, prepare samples, and calibrate the ESP32 code so that the `0–100` Display Index directly corresponds to actual Blood Alcohol Content (BAC).

---

## ⚙️ PART 1: Hardware Connections

**The 3-Electrode Strip:**
Your typical electrochemical sensor strip has three distinct pads:
1. **WE (Working Electrode)** — This is where the alcohol oxidation happens.
2. **RE (Reference Electrode)** — Maintains a stable comparison voltage.
3. **CE (Counter Electrode)** — Completes the circuit, occasionally tied to RE.

**Wiring to the MCP6002 Potentiostat Circuit:**
Follow this strict connection mapping:
* **Strip [WE]** ➔ Connects directly to **MCP6002 Pin 6** (U1B IN-)
* **Strip [RE]** ➔ Connects to the **0.5V Bias Node**: **MCP6002 Pin 1** (U1A OUT)
* **Strip [CE]** ➔ Tie this to **[RE]** for standard 2-electrode mode detection. (Connect CE to MCP6002 Pin 1).

> **WARNING:** Keep these wires *as short as possible*. Long wires act like antennas and pick up severe electrical noise. Crocodile clips attached directly near the board are best. 

---

## 🚦 PART 2: System Workflow (The State Machine)

The v10.0 code features a smart workflow to ensure perfect readings:

1. **BOOT & AUTO-CALIBRATING:** On startup, the system takes 10 samples to find the hardware baseline. Keep the sensor in your clean reference solution (e.g. clean water/saline).
2. **CHECKING (10s):** The system dynamically subtracts background conductivity and verifies circuit health. If wires are disconnected or shorted, it will display `CIRC ERROR!`.
3. **READY (Drop Now):** The system is monitoring for a jump of > 25nA. Simply add your alcohol drop/sample to the sensor to trigger it.
4. **SAMPLING (120s):** "ANALYSIS" phase. It captures the full electrochemical reaction curve for exactly 2 minutes to find the maximum peak current precisely.
5. **RESULT (15s):** The final calculated BAC % is locked on the screen and color-coded. 

---

## 🩸 PART 3: Preparing Calibration Test Solutions

To calibrate properly, you should test at least **one known concentration**.

Since obtaining laboratory-grade BAC simulation liquid is difficult, you can make a very close approximation using standard **40% ABV Vodka** or **70% Isopropyl Alcohol (Rubbing Alcohol)**.

> **Note on Volume vs Weight**: True BAC is measured by weight (grams of alcohol per 100mL of blood). For a DIY water-based test, mixing by volume gets us extremely close for sensor tuning.

### Option A: Using 40% ABV Vodka (Recommended, safer to handle)
*Vodka is essentially 40% ethanol and 60% water.*

**1. The 0.080% BAC Equivalent (The "Illegal" limit in many places)**
*   Pour **500 mL** of clean water into a measuring jug.
*   Add exactly **1 mL** of 40% Vodka.
*   *Math: (1 / 500) * 0.40 = 0.0008 = 0.08%*

**2. The 0.050% BAC Equivalent (The "Caution" level)**
*   Pour **500 mL** of clean water.
*   Add exactly **0.6 mL** of 40% Vodka (use a small syringe).

### Option B: Using 70% Isopropyl Alcohol (Rubbing Alcohol)
*If you don't have vodka, rubbing alcohol works perfectly for electrochemical sensors.*

**1. The 0.080% BAC Equivalent**
*   Pour **875 mL** of clean water.
*   Add exactly **1 mL** of 70% IPA.
*   *Math: (1 / 875) * 0.70 = 0.0008 = 0.08%*

### Option C: Using Pure Ethanol (99% - 100%)
*If you have access to laboratory-grade absolute ethanol.*

**1. The 0.080% BAC Equivalent**
*   Pour **1000 mL (1 Litre)** of clean water.
*   Add exactly **0.8 mL** of pure ethanol (using a micro-syringe).

*Stir the mixtures gently but thoroughly. Do not shake vigorously, as you don't want to dissolve excess oxygen into the water which can temporarily throw off electrochemical sensors.*

---

## 📊 PART 4: Performing the Calibration

*Note: The code currently has a default `SENSITIVITY_NA = 50.0`. You must tune this to your exact hardware.*

**Step 1. Run the Test**
1. Turn on the device and let it zero out in clean water during the `AUTO-CALIBRATING` and `CHECKING` phases.
2. When the screen shows **`DROP NOW`** (`READY` state), carefully swap the sensor into your prepared `0.080%` sample.
3. The display will jump to **`ANALYSIS`**. Wait the full 120 seconds. 
4. **Open the Arduino Serial Monitor (115200 baud).** 

**Step 2. Read the Summary & Calculate**
When the 120 seconds finish, the Serial Monitor will automatically print out a block like this:
```text
--- CALIBRATION TEST COMPLETE ---
Peak Current (nA): 400.00
Current Sensitivity: 50.00
Calculated BAC: 0.0800%
To adjust: SENSITIVITY_NA = Peak / (Real_BAC * 100)
----------------------------------
```
**If your Calculated BAC does not exactly match 0.0800%:**
1. Follow the exact formula the monitor gave you.
2. Example: If peak was `420 nA` on a `0.080%` test sample:
   * `420 / (0.080 * 100)` = `420 / 8` = **`52.5`**

**Step 3. Update the Code**
1. Open `AlcoSense_Pro_ESP32.ino`.
2. Find line 73: 
   ```cpp
   float SENSITIVITY_NA = 50.0; // UPDATED: Higher value (45-60)
   ```
3. Change `50.0` to your new calculated value (e.g. `52.5`).
4. Re-upload your code. Your system is perfectly calibrated!

---

## 🎨 PART 5: The Index Scale (0–100)

The display uses a `0.00` to `100.00` Index. 
* `0.00` = Absolutely zero alcohol detected.
* `100.00` = The physical maximum limit the system displays (0.300% BAC).

**Color Zones:**
* **` GREEN  | SOBER`** : Safe levels (< 0.060% BAC)
* **` MAGENTA| MEDIUM`** : Warning level (0.060% – 0.080% BAC)
* **` AMBER  | ILLEGAL`** : Over standard limits (0.080% – 0.150% BAC)
* **` RED    | DANGER!`** : Severe levels (> 0.150% BAC)
