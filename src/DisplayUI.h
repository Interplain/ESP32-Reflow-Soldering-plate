#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "Types.h"
#include "Profiles.h"  // <-- ADD THIS LINE - you're using Profile struct but not including it

// Minimal OLED UI for menu + run screens.
// You can extend with your profile graph later.
class DisplayUI {
public:
  // Begin I2C and the SSD1306. addr defaults to 0x3C.
  bool begin(uint8_t sda, uint8_t scl, uint8_t addr = 0x3C);

  // Simple menu: highlight selected index, show current temps.
  void showMenu(int index, float tFrontC, float tBackC, HeatState sel, bool fanMode, bool fanState);
  // Generic run screen used for both Constant and Profile modes.
  // Shows Set Point, temps, and heater duties.
  void showRun(float spC, float tFrontC, float tBackC, int dutyFrontPct, int dutyBackPct, Mode mode);

   // ADD THESE NEW METHODS:
  void setupProfileDisplay(const Profile& prof, int durationSec);
  void showProfileRun(const Profile& prof, float sp, float tF, float tB, 
                      int dutyF, int dutyB, int elapsed, int remaining, 
                      bool done, bool aborted);
  void plotTemperature(float avgTemp, int currentSec);

  // Add these to your DisplayUI.h public section:
void showProfileSetup(const Profile& prof, int plateCount);
void showConstantSetup(int targetTemp, int duration);
void showTest(int dutyCycle, float tF, float tB, HeatState heatSel);
void showCoolTest(float tF, float tB);

  // Optional helper to clear screen.
  void clear();

private:
  Adafruit_SSD1306 d_{128, 64, &Wire, -1};
  void drawPlateIconsAt_(int x, int y, HeatState sel, bool blinkOn);
  
  // ADD THESE NEW PRIVATE MEMBERS:
  const Profile* currentProfile_ = nullptr;
  int profileDuration_ = 0;
  float secsPerDispSlot_ = 0;
  int lastPlotX_ = -1;
  uint8_t setPointDisp_[108];  // Setpoint outline for display
  
  // ADD THESE NEW PRIVATE METHODS:
  void drawCondensedProfileOutline_(const Profile& prof);
};