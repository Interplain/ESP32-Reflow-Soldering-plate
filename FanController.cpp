#include "FanController.h"

bool FanController::begin(uint8_t pin, bool activeLow, uint32_t pwmHz, uint8_t pwmResBits){
  pin_       = pin;
  activeLow_ = activeLow;
  freq_      = pwmHz;
  resBits_   = pwmResBits;
  maxRaw_    = (1u << resBits_) - 1u;
  lastRaw_   = 0;

  // Attach wrapper LEDC directly to the pin (ESP32 core 3.x)
  if (!ledcAttach(pin_, freq_, resBits_)) {
    // If attach fails, ensure pin is at OFF level so hardware stays safe
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, activeLow_ ? HIGH : LOW);
    return false;
  }

  apply_(); // write OFF
  return true;
}

void FanController::apply_(){
  uint32_t raw = lastRaw_;
  if (activeLow_) raw = maxRaw_ - raw;  // invert if LOW=ON hardware
  ledcWrite(pin_, raw);
}

void FanController::set(bool on){
  lastRaw_ = on ? maxRaw_ : 0u;
  apply_();
}

void FanController::setDutyPct(uint8_t pct){
  if (pct > 100) pct = 100;
  lastRaw_ = (uint32_t)((pct * maxRaw_ + 50) / 100); // rounded
  apply_();
}

void FanController::hysteresis(float maxTempC, float onC, float offC, uint8_t dutyPctOn){
  if (onC < offC) onC = offC; // sanity guard
  // OFF -> ON
  if (lastRaw_ == 0 && maxTempC >= onC) {
    setDutyPct(dutyPctOn);
    Serial.println(F("[FAN] ON"));
  }
  // ON -> OFF
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
