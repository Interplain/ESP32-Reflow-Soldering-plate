#pragma once
#include <Arduino.h>

// Minimal PWM fan controller using ESP32 LEDC wrapper API.
// Use for a 2-wire fan driven by a opto isolator on a GPIO19.
// Default: activeLow=false (HIGH = ON), 100 Hz, 12-bit duty (0..4095).
class FanController {
public:
  // pin: GPIO to drive opto isolator
  // activeLow: set true if your hardware turns ON when GPIO is LOW
  // pwmHz: usually 100 Hz for a 2-wire fan power PWM (or just treat as ON/OFF)
  // pwmResBits: 12 -> 0..4095 duty
  bool begin(uint8_t pin, bool activeLow=false, uint32_t pwmHz=100, uint8_t pwmResBits=12);

  // ON (100%) / OFF (0%)
  void set(bool on);

  // 0..100% duty (rounded to LEDC raw). In 2-wire fans this is "power PWM".
  void setDutyPct(uint8_t pct);

  // Simple hysteresis: ON at >= onC (dutyPctOn), OFF at <= offC
  void hysteresis(float maxTempC, float onC=80.0f, float offC=75.0f, uint8_t dutyPctOn=100);

  // Boot self-test
  void selfTest(uint16_t msOn=1500);

  bool isOn() const { return lastRaw_ > 0; }

private:
  void apply_();

  uint8_t  pin_        = 255;
  bool     activeLow_  = false;
  uint8_t  resBits_    = 12;
  uint32_t freq_       = 100;
  uint32_t maxRaw_     = 4095;
  uint32_t lastRaw_    = 0;
};
