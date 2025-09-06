#pragma once
#include <Arduino.h>

struct ProfileEntry { uint16_t slotSecs; uint16_t targetTempC; };
#define MAXPRSLOTS 10

struct Profile {
  const char *name;
  uint8_t     pidProfile;   // kept for future
  uint8_t     slotCount;
  uint8_t     coolingSlot;  // index where cooling begins (optional)
  int         profMinTemp;
  int         profMaxTemp;
  int         doneTemp;
  ProfileEntry slots[MAXPRSLOTS];
};

extern const Profile PROFILES[];
extern const uint8_t PROFILE_COUNT;
