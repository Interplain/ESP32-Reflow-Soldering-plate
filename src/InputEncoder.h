// InputEncoder.h
#pragma once
#include <Arduino.h>

struct InputEvents { 
    int steps; 
    bool click; 
    bool longPress; 
};

class InputEncoder {
public:
    void begin(uint8_t pinA, uint8_t pinB, uint8_t pinBtn, bool btnActiveLow = true);
    InputEvents poll();

private:
    static InputEncoder* instance_;
    static void IRAM_ATTR encoderISR();
    
    uint8_t pinA_, pinB_, pinBtn_;
    bool btnActiveLow_;
    
    // Encoder state
    static volatile int8_t encoderValue_;
    static volatile uint8_t oldAB_;
    
    // Button debounce
    unsigned long lastButtonTime_ = 0;
    unsigned long buttonPressStart_ = 0;
    bool buttonPressed_ = false;
    bool buttonHandled_ = false;
    
    static const int8_t ENC_STATES_[16];
    static const unsigned long DEBOUNCE_MS = 100;
    static const unsigned long LONG_PRESS_MS = 1000;
};
