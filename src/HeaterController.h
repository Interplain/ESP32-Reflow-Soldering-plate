// HeaterController.h
#pragma once
#include <Arduino.h>
#include "Types.h"

class HeaterController {
public:
    void begin(uint8_t ssrFrontPin, uint8_t ssrBackPin, unsigned long windowMs = 1000);
    void reset();
    void setGains(const PIDGains& gains);
    void control(HeatState selection, float setpoint, float tempFront, float tempBack);
    
    // Status reporting
    int dutyFrontPct() const { return dutyFront_; }
    int dutyBackPct() const { return dutyBack_; }
    float getLastError() const { return lastError_; }
    
    // Safety and tuning
    void setMaxOutput(int maxPct) { maxOutputPct_ = constrain(maxPct, 0, 100); }
    void enableDebug(bool enable) { debugEnabled_ = enable; }

private:
    // Hardware pins
    uint8_t pinFront_, pinBack_;
    
    // Timing control for SSR time-proportioning
    unsigned long windowMs_;
    unsigned long windowStartFront_, windowStartBack_;
    
    // PID parameters
    PIDGains gains_;
    int maxOutputPct_;
    
    // PID state - Front channel
    float errorIntegralFront_;
    float errorPrevFront_;
    unsigned long lastTimeFront_;
    
    // PID state - Back channel  
    float errorIntegralBack_;
    float errorPrevBack_;
    unsigned long lastTimeBack_;
    
    // Output duty cycles (0-100%)
    int dutyFront_;
    int dutyBack_;
    
    // Debug and monitoring
    bool debugEnabled_;
    float lastError_;
    unsigned long lastDebugTime_;
    
    // Internal methods
    void drivePin(uint8_t pin, bool state);
    int calculatePID(float setpoint, float processValue, 
                    float& errorIntegral, float& errorPrev, unsigned long& lastTime);
    void printDebugInfo(float setpoint, float tempFront, float tempBack, HeatState selection);
};
