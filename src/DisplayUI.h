#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "Types.h"

// Minimal OLED UI for menu + run screens.
// You can extend with your profile graph later.
class DisplayUI {
public:
  // Begin I2C and the SSD1306. addr defaults to 0x3C.
  bool begin(uint8_t sda, uint8_t scl, uint8_t addr = 0x3C);

  // Simple menu: highlight selected index, show current temps.
  void showMenu(int index, float tFrontC, float tBackC, HeatState sel);

  // Generic run screen used for both Constant and Profile modes.
  // Shows Set Point, temps, and heater duties.
  void showRun(float spC, float tFrontC, float tBackC, int dutyFrontPct, int dutyBackPct, Mode mode);

  // Optional helper to clear screen.
  void clear();

private:
  Adafruit_SSD1306 d_{128, 64, &Wire, -1};
  void drawPlateIconsAt_(int x, int y, HeatState sel, bool blinkOn);
};
