#pragma once
#include "Profiles.h"

class ProfileRunner {
public:
  void begin(const Profile& p);
  // returns current setpoint (°C) and whether finished
  float update(uint32_t nowMs, bool& finished);
  uint16_t elapsedSec(uint32_t nowMs) const;
  uint16_t durationSec() const { return durnSec_; }
  uint16_t coolingStartSec() const { return coolingStartSec_; }

private:
  const Profile* prof_ = nullptr;
  uint32_t startMs_ = 0;
  uint16_t durnSec_ = 0;
  uint16_t coolingStartSec_ = 0;
  // optional: cache ‘outline’ for display if you like later
};
