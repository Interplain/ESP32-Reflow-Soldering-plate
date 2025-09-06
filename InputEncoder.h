#pragma once
#include <Arduino.h>

// High-level events from a single rotary encoder + pushbutton
struct InputEvents {
  int  steps;       // positive = clockwise detents, negative = ccw
  bool click;       // short press
  bool longPress;   // long press (also sets click=true on release)
};

// Single-instance encoder driver (one encoder per firmware).
// Uses two interrupts on A/B for smooth stepping and polled debounce for the button.
class InputEncoder {
public:
  // btnActiveLow=true for typical modules with pull-ups (LOW when pressed)
  void begin(uint8_t pinA, uint8_t pinB, uint8_t pinBtn, bool btnActiveLow = true);

  // Drain accumulated detents from ISR buffer.
  int readSteps();

  // Poll this fast (e.g., every loop) to get button events + steps atomically.
  InputEvents poll();

private:
  friend void IRAM_ATTR _enc_isrA();
  friend void IRAM_ATTR _enc_isrB();

  // pins / config
  uint8_t a_=255, b_=255, btn_=255;
  bool btnActiveLow_=true;

  // debounce
  bool btnRawPrev_=false, btnStable_=false, btnWasPressed_=false;
  unsigned long btnChangeMs_=0, btnPressStartMs_=0;

  // This class is single-instance; we keep a static pointer the ISRs can use.
  static InputEncoder* self_;
};
