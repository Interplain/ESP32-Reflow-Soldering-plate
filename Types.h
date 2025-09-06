#pragma once
#include <Arduino.h>

enum HeatState : uint8_t { HEAT_OFF, HEAT_BOTH, HEAT_FRONT, HEAT_BACK };
enum Mode : uint8_t { MENU, PROF_SETUP, PROFILE_RUN, CONST_SETUP, CONST_RUN, TEST_RUN };

struct PIDGains { float P, I, D; float iMax; };
