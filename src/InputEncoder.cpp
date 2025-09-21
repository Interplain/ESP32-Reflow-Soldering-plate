//-------- InputEncoder.cpp --------
#include "InputEncoder.h"

// Static member definitions (only define once!)
InputEncoder* InputEncoder::instance_ = nullptr;
volatile int8_t InputEncoder::encoderValue_ = 0;
volatile uint8_t InputEncoder::oldAB_ = 3;

const int8_t InputEncoder::ENC_STATES_[16] = {
    0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0
};

void InputEncoder::begin(uint8_t pinA, uint8_t pinB, uint8_t pinBtn, bool btnActiveLow) {
    pinA_ = pinA;
    pinB_ = pinB;
    pinBtn_ = pinBtn;
    btnActiveLow_ = btnActiveLow;
    
    pinMode(pinA_, INPUT);
    pinMode(pinB_, INPUT);
    pinMode(pinBtn_, INPUT_PULLUP);
    
    encoderValue_ = 0;
    oldAB_ = 3;
    if (digitalRead(pinA_)) oldAB_ |= 0x02;
    if (digitalRead(pinB_)) oldAB_ |= 0x01;
    
    instance_ = this;
    attachInterrupt(digitalPinToInterrupt(pinA_), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(pinB_), encoderISR, CHANGE);
    
    Serial.println("Encoder initialized");
}

void IRAM_ATTR InputEncoder::encoderISR() {
    if (!instance_) return;
    
    oldAB_ <<= 2;
    if (digitalRead(instance_->pinA_)) oldAB_ |= 0x02;
    if (digitalRead(instance_->pinB_)) oldAB_ |= 0x01;
    
    encoderValue_ += ENC_STATES_[(oldAB_ & 0x0f)];
}

InputEvents InputEncoder::poll() {
    InputEvents events = {0, false, false};
    
    // Read encoder steps (with detent filtering)
    if (encoderValue_ > 3 || encoderValue_ < -3) {
        events.steps = (encoderValue_ > 0) ? 1 : -1;
        encoderValue_ = 0;
    }
    
    // Handle button
    bool currentButtonState = digitalRead(pinBtn_);
    if (btnActiveLow_) currentButtonState = !currentButtonState;
    
    unsigned long now = millis();
    
    if (currentButtonState && !buttonPressed_) {
        if (now - lastButtonTime_ > DEBOUNCE_MS) {
            buttonPressed_ = true;
            buttonPressStart_ = now;
            buttonHandled_ = false;
        }
    } else if (!currentButtonState && buttonPressed_) {
        if (now - lastButtonTime_ > DEBOUNCE_MS && !buttonHandled_) {
            unsigned long pressDuration = now - buttonPressStart_;
            if (pressDuration < LONG_PRESS_MS) {
                events.click = true;
            } else {
                events.longPress = true;
            }
            buttonHandled_ = true;
        }
        buttonPressed_ = false;
        lastButtonTime_ = now;
    }
    
    return events;
}
