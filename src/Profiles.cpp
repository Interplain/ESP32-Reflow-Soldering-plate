#include "Profiles.h"

// Your 7 profiles from the big sketch:
const Profile PROFILES[] = {
  // Low temp - Chipquik 165°C (7 minutes total)
  {"Chipqk 165C", 0, 6, 4, 10, 170, 30, {
    {90,45},      // Preheat
    {180,130},    // Soak
    {240,165},    // Peak at 4 min
    {330,100},    // Cool 90s (realistic based on test)
    {390,60},     // 
    {420,35}      // Safe
  }},
  
  // Mid temp - 200°C (9 minutes total)
  {"Lead 200C", 0, 6, 3, 10, 210, 30, {
    {120,100},    
    {200,150},    
    {250,200},    // Peak at slot 3
    {370,120},    // Cool 2 min (from data: 200→120 ≈ 2min)
    {480,70},     // 
    {540,40}      // Safe
  }},
  
  // High temp - 230°C (12 minutes total)  
  {"High 230C", 0, 7, 4, 10, 240, 30, {
    {90,90},      
    {180,130},    
    {210,165},    
    {240,230},    // Peak - slot 4
    {360,150},    // Cool 2 min
    {510,90},     // 2.5 min more
    {720,40}      // Safe (8 min cooling total)
  }},
  
  // Quick test 100°C (4 minutes)
  {"Test 100C", 0, 4, 2, 25, 110, 30, {
    {60,50},      
    {120,100},    // Peak
    {240,60},     // Cool 2 min
    {300,35}      // Done
  }},
  
  // Step test for PID tuning
  {"Step Test", 0, 3, 3, 25, 100, 30, {
    {30,50},      
    {60,80},      
    {180,40}      // Cool at slot 3
  }}
};
const uint8_t PROFILE_COUNT = sizeof(PROFILES)/sizeof(PROFILES[0]);
