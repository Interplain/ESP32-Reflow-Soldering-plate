#include "HeaterController.h"

void HeaterController::begin(uint8_t f, uint8_t b, unsigned long w){
  pF_=f; pB_=b; winMs_=w;
  pinMode(pF_,OUTPUT); pinMode(pB_,OUTPUT);
  drive_(pF_,false); drive_(pB_,false);
  winStartF_=winStartB_=millis();
  lastMsF_=lastMsB_=millis();
  lastSp_ = NAN;
}
void HeaterController::reset(){
  eIntF_=ePrevF_=eIntB_=ePrevB_=0;
  lastMsF_=lastMsB_=millis();
  drive_(pF_,false); drive_(pB_,false);
  lastSp_ = NAN;
}

// PID with conditional integration to avoid windup
int HeaterController::pidPct_(float sp, float pv, float& eInt, float& ePrev, unsigned long& lastMs){
  unsigned long now=millis();
  float dt=(now-lastMs)/1000.0f; if(dt<=0) dt=0.001f; lastMs=now;

  float e = sp - pv;
  float d = (e - ePrev)/dt;
  float iCand = eInt + e*dt;

  // raw output before clamping, using candidate I
  float u = gains_.P*e + gains_.I*iCand + gains_.D*d;

  // if driving into negative (cooling) or saturating high/low, control integration
  const float CAP = 400.0f;
  bool wouldClampLow  = (u <= 0);
  bool wouldClampHigh = (u >= CAP);

  // integrate only if not saturating or if the error would drive us out of saturation
  if (!wouldClampLow && !wouldClampHigh) {
    eInt = iCand;
  } else if (wouldClampHigh && e < 0) {
    eInt = iCand; // allow wind-down from top
  } else if (wouldClampLow && e > 0) {
    eInt = iCand; // allow wind-up from bottom back toward control range
  }
  // clamp integral
  if (eInt > gains_.iMax) eInt = gains_.iMax;
  if (eInt < -gains_.iMax) eInt = -gains_.iMax;

  // recompute with accepted integral
  u = gains_.P*e + gains_.I*eInt + gains_.D*d;

  if (u < 0) u = 0;
  int pct = (u>=CAP)?100:int(u*100.0f/CAP + 0.5f);
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  ePrev = e;
  return pct;
}

void HeaterController::control(HeatState sel, float sp, float tF, float tB){
  unsigned long now=millis();

  // If setpoint decreased materially, zero integrators (bumpless transfer)
  if (!isnan(lastSp_) && sp < lastSp_ - 0.5f) { // 0.5°C threshold
    eIntF_=ePrevF_=0; eIntB_=ePrevB_=0;
  }
  lastSp_ = sp;

  // FRONT channel
  if (sel==HEAT_FRONT || sel==HEAT_BOTH) {
    // cooling guard: don't add heat when at/above SP - band
    if (tF >= sp - offBandC_) {
      dutyF_ = 0; eIntF_=0; ePrevF_=0;
      drive_(pF_, false);
    } else {
      dutyF_ = pidPct_(sp, tF, eIntF_, ePrevF_, lastMsF_);
      if (now - winStartF_ >= winMs_) winStartF_ += winMs_;
      bool on = ((now - winStartF_) < (unsigned long)(dutyF_ * winMs_ / 100));
      drive_(pF_, on);
    }
  } else {
    dutyF_ = 0; drive_(pF_, false);
  }

  // BACK channel
  if (sel==HEAT_BACK || sel==HEAT_BOTH) {
    if (tB >= sp - offBandC_) {
      dutyB_ = 0; eIntB_=0; ePrevB_=0;
      drive_(pB_, false);
    } else {
      dutyB_ = pidPct_(sp, tB, eIntB_, ePrevB_, lastMsB_);
      if (now - winStartB_ >= winMs_) winStartB_ += winMs_;
      bool on = ((now - winStartB_) < (unsigned long)(dutyB_ * winMs_ / 100));
      drive_(pB_, on);
    }
  } else {
    dutyB_ = 0; drive_(pB_, false);
  }
}
