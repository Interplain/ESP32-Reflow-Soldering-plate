#include "ProfileRunner.h"
#include <Arduino.h>

void ProfileRunner::begin(const Profile& p){
  prof_ = &p;
  startMs_ = millis();
  durnSec_ = p.slots[p.slotCount-1].slotSecs;
  coolingStartSec_ = (p.coolingSlot < p.slotCount) ? p.slots[p.coolingSlot].slotSecs : durnSec_;
}

uint16_t ProfileRunner::elapsedSec(uint32_t nowMs) const {
  if(!startMs_) return 0;
  uint32_t s = (nowMs - startMs_) / 1000U;
  return (s > durnSec_) ? durnSec_ : (uint16_t)s;
}

float ProfileRunner::update(uint32_t nowMs, bool& finished){
  finished = false;
  if(!prof_) return 25.0f;
  uint16_t sec = elapsedSec(nowMs);
  if (sec >= durnSec_) { finished = true; sec = durnSec_ ? (durnSec_-1) : 0; }

  const Profile& P = *prof_;
  float curT = P.profMinTemp;
  uint16_t prevT = 0;

  for (uint8_t s = 0; s < P.slotCount; ++s) {
    uint16_t t = P.slots[s].slotSecs;
    float    y = P.slots[s].targetTempC;
    if (sec <= t) {
      uint16_t dt = t - prevT;
      if (dt == 0) return y;
      float frac = float(sec - prevT) / float(dt);
      return curT + frac * (y - curT);
    }
    curT = y; prevT = t;
  }
  return curT;
}
