#include "FanController.h"

#ifndef ESP_ARDUINO_VERSION_MAJOR
  #define ESP_ARDUINO_VERSION_MAJOR 2
#endif

bool FanController::begin(uint8_t pin, bool activeLow, uint32_t pwmHz, uint8_t pwmResBits){
  pin_       = pin;
  activeLow_ = activeLow;
  freq_      = pwmHz;
  resBits_   = pwmResBits;
  maxRaw_    = (1u << resBits_) - 1u;
  lastRaw_   = 0;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // Core 3.x: pin API
  if (!ledcAttach(pin_, freq_, resBits_)) {
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, activeLow_ ? HIGH : LOW); // OFF level at fan
    return false;
  }
  usePinAPI_ = true;
#else
  // Core 2.x: channel API
  channel_ = 6; // pick a dedicated channel to avoid clashes
  if (ledcSetup(channel_, freq_, resBits_) == 0) {
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, activeLow_ ? HIGH : LOW); // OFF level at fan
    return false;
  }
  ledcAttachPin(pin_, channel_);
  usePinAPI_ = false;
#endif

  // IMPORTANT: apply OFF via our polarity-aware path
  lastRaw_ = 0;     // logical OFF
  apply_();         // writes with correct inversion

  return true;
}

void FanController::apply_(){
  uint32_t raw = lastRaw_;
  if (activeLow_) raw = maxRaw_ - raw;  // LOW at fan = ON

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin_, raw);          // pin API
#else
  if (channel_ >= 0) ledcWrite(channel_, raw); // channel API
#endif
}

void FanController::set(bool on){
  lastRaw_ = on ? maxRaw_ : 0u;
  apply_();
}

void FanController::setDutyPct(uint8_t pct){
  if (pct > 100) pct = 100;
  lastRaw_ = (uint32_t)((uint64_t)pct * (uint64_t)maxRaw_ + 50) / 100;
  apply_();
}

void FanController::coolDown(float currentTemp, float setpoint, float tolerance) {
  if (currentTemp > setpoint + tolerance) {
    if (lastRaw_ == 0) {
      setDutyPct(100);
      Serial.println(F("[FAN] Cooling ON"));
    }
  } else if (currentTemp <= setpoint) {
    if (lastRaw_ > 0) {
      set(false);
      Serial.println(F("[FAN] Cooling complete"));
    }
  }
}

void FanController::hysteresis(float maxTempC, float onC, float offC, uint8_t dutyPctOn){
  if (onC < offC) onC = offC;
  if (lastRaw_ == 0 && maxTempC >= onC) {
    setDutyPct(dutyPctOn);
    Serial.println(F("[FAN] ON"));
  }
  if (lastRaw_ > 0 && maxTempC <= offC) {
    set(false);
    Serial.println(F("[FAN] OFF"));
  }
}

void FanController::selfTest(uint16_t msOn){
  set(true);
  delay(msOn);
  set(false);
}
