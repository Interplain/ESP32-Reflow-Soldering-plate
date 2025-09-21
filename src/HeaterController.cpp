// HeaterController.cpp
#include "HeaterController.h"

void HeaterController::begin(uint8_t ssrFrontPin, uint8_t ssrBackPin, unsigned long windowMs) {
    pinFront_ = ssrFrontPin;
    pinBack_ = ssrBackPin;
    windowMs_ = windowMs;
    
    // Initialize GPIO pins
    pinMode(pinFront_, OUTPUT);
    pinMode(pinBack_, OUTPUT);
    drivePin(pinFront_, false);
    drivePin(pinBack_, false);
    
    // Initialize timing
    windowStartFront_ = millis();
    windowStartBack_ = millis();
    lastTimeFront_ = millis();
    lastTimeBack_ = millis();
    lastDebugTime_ = 0;
    
    // Initialize PID state
    reset();
    
    // Default parameters
    gains_ = {6.0f, 0.15f, 3.0f, 150.0f};  // Conservative defaults
    maxOutputPct_ = 90;  // Safety limit - don't run SSRs at 100%
    debugEnabled_ = false;
    
    Serial.println("HeaterController: Initialized");
    Serial.printf("Front pin: %d, Back pin: %d, Window: %lums\n", 
                  pinFront_, pinBack_, windowMs_);
}

void HeaterController::reset() {
    // Clear all PID states
    errorIntegralFront_ = 0.0f;
    errorPrevFront_ = 0.0f;
    errorIntegralBack_ = 0.0f;
    errorPrevBack_ = 0.0f;
    
    // Reset timing
    lastTimeFront_ = millis();
    lastTimeBack_ = millis();
    
    // Turn off outputs
    dutyFront_ = 0;
    dutyBack_ = 0;
    drivePin(pinFront_, false);
    drivePin(pinBack_, false);
    
    lastError_ = 0.0f;
    
    Serial.println("HeaterController: Reset - All outputs OFF, PID states cleared");
}

void HeaterController::setGains(const PIDGains& gains) {
    gains_ = gains;
    
    // Reset integral terms when gains change to prevent instability
    errorIntegralFront_ = 0.0f;
    errorIntegralBack_ = 0.0f;
    
    Serial.printf("HeaterController: PID gains updated - P:%.2f I:%.2f D:%.2f IMax:%.1f\n",
                  gains_.P, gains_.I, gains_.D, gains_.iMax);
}

void HeaterController::control(HeatState selection, float setpoint, float tempFront, float tempBack) {
    unsigned long now = millis();
    
    // Front heater control
    if (selection == HEAT_FRONT || selection == HEAT_BOTH) {
        dutyFront_ = calculatePID(setpoint, tempFront, errorIntegralFront_, 
                                  errorPrevFront_, lastTimeFront_);
        
        // Fixed window timing - don't let windows drift
        if (now - windowStartFront_ >= windowMs_) {
            windowStartFront_ += windowMs_;  // Maintain exact timing
        }
        
        unsigned long onTime = (dutyFront_ * windowMs_) / 100;
        bool outputOn = (now - windowStartFront_) < onTime;
        drivePin(pinFront_, outputOn);
        
    } else {
        dutyFront_ = 0;
        drivePin(pinFront_, false);
        errorIntegralFront_ = 0;
    }
    
    // Back heater control (same fix)
    if (selection == HEAT_BACK || selection == HEAT_BOTH) {
        dutyBack_ = calculatePID(setpoint, tempBack, errorIntegralBack_, 
                                 errorPrevBack_, lastTimeBack_);
        
        if (now - windowStartBack_ >= windowMs_) {
            windowStartBack_ += windowMs_;  // Fixed timing
        }
        
        unsigned long onTime = (dutyBack_ * windowMs_) / 100;
        bool outputOn = (now - windowStartBack_) < onTime;
        drivePin(pinBack_, outputOn);
        
    } else {
        dutyBack_ = 0;
        drivePin(pinBack_, false);
        errorIntegralBack_ = 0;
    }
    
    // Debug every 2 seconds
    static unsigned long lastDebug = 0;
    if (now - lastDebug > 2000) {
        Serial.printf("SP:%.1f F:%.1f(%.0f%%) B:%.1f(%.0f%%)\n", 
                      setpoint, tempFront, dutyFront_, tempBack, dutyBack_);
        lastDebug = now;
    }
}
int HeaterController::calculatePID(float setpoint, float processValue, 
                                  float& errorIntegral, float& errorPrev, unsigned long& lastTime) {
    unsigned long now = millis();
    float deltaTime = (now - lastTime) / 1000.0f;
    
    // Prevent division by zero and handle timer rollover
    if (deltaTime <= 0 || deltaTime > 10.0f) {
        deltaTime = 0.001f;
    }
    lastTime = now;
    
    // Calculate error
    float error = setpoint - processValue;
    lastError_ = error;  // Store for monitoring
    
    // Proportional term
    float proportional = gains_.P * error;
    
    // Integral term with windup protection
    errorIntegral += error * deltaTime;
    
    // Clamp integral to prevent windup
    if (errorIntegral > gains_.iMax) {
        errorIntegral = gains_.iMax;
    } else if (errorIntegral < -gains_.iMax) {
        errorIntegral = -gains_.iMax;
    }
    
    float integral = gains_.I * errorIntegral;
    
    // Derivative term
    float derivative = gains_.D * (error - errorPrev) / deltaTime;
    errorPrev = error;
    
    // Calculate total output
    float output = proportional + integral + derivative;
    
    // Convert to percentage and clamp
    int outputPct = (int)(output + 0.5f);  // Round to nearest integer
    outputPct = constrain(outputPct, 0, maxOutputPct_);
    
    return outputPct;
}

void HeaterController::drivePin(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

void HeaterController::printDebugInfo(float setpoint, float tempFront, float tempBack, HeatState selection) {
    unsigned long now = millis();
    
    // Limit debug output to once per second
    if (now - lastDebugTime_ < 1000) {
        return;
    }
    lastDebugTime_ = now;
    
    Serial.printf("PID: SP=%.1f°C", setpoint);
    
    if (selection == HEAT_FRONT || selection == HEAT_BOTH) {
        Serial.printf(" F=%.1f°C(%.0f%%)", tempFront, dutyFront_);
    }
    
    if (selection == HEAT_BACK || selection == HEAT_BOTH) {
        Serial.printf(" B=%.1f°C(%.0f%%)", tempBack, dutyBack_);
    }
    
    Serial.printf(" Err=%.1f°C", lastError_);
    
    const char* modeStr;
    switch (selection) {
        case HEAT_OFF:   modeStr = "OFF"; break;
        case HEAT_FRONT: modeStr = "FRONT"; break;
        case HEAT_BACK:  modeStr = "BACK"; break;
        case HEAT_BOTH:  modeStr = "BOTH"; break;
        default:         modeStr = "UNKNOWN"; break;
    }
    Serial.printf(" Mode=%s\n", modeStr);
}
