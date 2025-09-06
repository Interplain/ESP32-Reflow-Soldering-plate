#pragma once
#include <Arduino.h>
#include "Types.h"

class HeaterController {
public:
  void begin(uint8_t ssrFrontPin, uint8_t ssrBackPin, unsigned long windowMs=1000);
  void reset();
  void setGains(const PIDGains& g){ gains_=g; }
  // Cooling band: if PV >= SP - band, output is forced OFF and I term is cleared
  void setCoolBand(float bandC){ offBandC_ = bandC; }
  void control(HeatState sel, float spC, float tFront, float tBack);
  int dutyFrontPct() const { return dutyF_; }
  int dutyBackPct()  const { return dutyB_; }

private:
  uint8_t pF_, pB_;
  unsigned long winMs_=1000, winStartF_=0, winStartB_=0;
  PIDGains gains_{6,0.15f,8,150};

  // per-channel PID state
  float eIntF_=0, ePrevF_=0;
  float eIntB_=0, ePrevB_=0;
  unsigned long lastMsF_=0, lastMsB_=0;
  int dutyF_=0, dutyB_=0;

  // anti-heat guards
  float offBandC_ = 1.5f;   // °C band where we force OFF to avoid adding heat
  float lastSp_   = NAN;    // to detect setpoint drops (bumpless transfer)

  void drive_(uint8_t pin, bool on){ digitalWrite(pin, on?HIGH:LOW); }
  int pidPct_(float sp, float pv, float& eInt, float& ePrev, unsigned long& lastMs);
};
