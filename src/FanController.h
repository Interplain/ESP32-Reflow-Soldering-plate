#pragma once
#include <Arduino.h>

// Minimal PWM fan controller for ESP32 LEDC.
// - Works with 4-wire PC fans via open-collector/NPN/opto on the PWM control line.
// - Default assumes active-LOW PWM at the fan input (LOW = ON), 25 kHz, 8-bit.
//
// Public API kept compatible with your code.
class FanController {
public:
  // Initialize PWM on 'pin'.
  // activeLow=true means LOW duty = ON at the fan input (typical).
  bool begin(uint8_t pin, bool activeLow = true, uint32_t pwmHz = 25000, uint8_t pwmResBits = 8);

  // Simple controls
  void set(bool on);                 // 0% or 100%
  void setDutyPct(uint8_t pct);      // 0..100

  // Helpers
  void hysteresis(float maxTempC, float onC = 80.0f, float offC = 75.0f, uint8_t dutyPctOn = 100);
  void coolDown(float currentTemp, float setpoint, float tolerance = 5.0f);
  void selfTest(uint16_t msOn = 1500);

  bool isOn() const { return lastRaw_ > 0; }

private:
  void apply_();

  // Configuration / state
  uint8_t  pin_        = 255;
  bool     activeLow_  = true;
  uint8_t  resBits_    = 8;
  uint32_t freq_       = 25000;
  uint32_t maxRaw_     = 255;
  uint32_t lastRaw_    = 0;

  // Backend selection
  int8_t   channel_    = -1;   // used on core 2.x
  bool     usePinAPI_  = false; // true on core 3.x
};

