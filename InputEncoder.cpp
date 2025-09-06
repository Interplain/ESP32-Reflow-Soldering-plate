#include "InputEncoder.h"

// ======= Internal single-instance state used by ISRs =======
InputEncoder* InputEncoder::self_ = nullptr;

// Quadrature table (gray code)
static constexpr int8_t QTAB[16] = {
  0,-1,+1, 0,
 +1, 0, 0,-1,
 -1, 0, 0,+1,
  0,+1,-1, 0
};

// Accumulator for encoder transitions (quarter-steps)
static volatile int32_t g_qsum = 0;
static volatile uint8_t g_prev = 0x03;  // idle (A=1,B=1)

// One detent = 4 transitions on most mechanical encoders
static constexpr int ENC_DETENT = 4;

// Read both pins quickly
static inline uint8_t _readAB(uint8_t aPin, uint8_t bPin) {
  uint8_t a = (uint8_t)digitalRead(aPin);
  uint8_t b = (uint8_t)digitalRead(bPin);
  return (uint8_t)((a << 1) | b);
}

// Shared ISR body
static inline void _enc_isr_body(uint8_t aPin, uint8_t bPin) {
  uint8_t s   = _readAB(aPin, bPin);
  uint8_t idx = ((g_prev & 0x03) << 2) | (s & 0x03);
  int8_t delta = QTAB[idx];
  g_prev = s;
  if (delta) g_qsum += delta;
}

// Actual attached ISRs
void IRAM_ATTR _enc_isrA() { if (InputEncoder::self_) _enc_isr_body(InputEncoder::self_->a_, InputEncoder::self_->b_); }
void IRAM_ATTR _enc_isrB() { if (InputEncoder::self_) _enc_isr_body(InputEncoder::self_->a_, InputEncoder::self_->b_); }

void InputEncoder::begin(uint8_t pinA, uint8_t pinB, uint8_t pinBtn, bool btnActiveLow) {
  a_ = pinA; b_ = pinB; btn_ = pinBtn; btnActiveLow_ = btnActiveLow;

  pinMode(a_, INPUT);
  pinMode(b_, INPUT);
  pinMode(btn_, btnActiveLow_ ? INPUT_PULLUP : INPUT);

  // Initialize prev state so first transition indexes correctly
  g_prev = _readAB(a_, b_);
  g_qsum = 0;

  // Register instance so ISRs can access pins
  self_ = this;

  attachInterrupt(digitalPinToInterrupt(a_), _enc_isrA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(b_), _enc_isrB, CHANGE);

  // Button initial states
  bool raw = (digitalRead(btn_) == (btnActiveLow_ ? LOW : HIGH));
  btnRawPrev_ = raw;
  btnStable_  = raw;
  btnWasPressed_ = false;
  btnChangeMs_   = millis();
  btnPressStartMs_ = 0;
}

int InputEncoder::readSteps() {
  // Convert quarter-steps to detent steps atomically
  int steps = 0;
  noInterrupts();
  while (g_qsum >= ENC_DETENT) { ++steps; g_qsum -= ENC_DETENT; }
  while (g_qsum <= -ENC_DETENT){ --steps; g_qsum += ENC_DETENT; }
  interrupts();
  return steps;
}

InputEvents InputEncoder::poll() {
  // Button debounce & long-press; also drains detents now
  static const uint32_t DEBOUNCE_MS = 10;
  static const uint32_t LONG_MS     = 900;

  InputEvents ev{0,false,false};

  // 1) steps
  ev.steps = readSteps();

  // 2) button debounce
  bool raw = (digitalRead(btn_) == (btnActiveLow_ ? LOW : HIGH));
  if (raw != btnRawPrev_) {
    btnRawPrev_ = raw;
    btnChangeMs_ = millis();
  }

  if ((millis() - btnChangeMs_) > DEBOUNCE_MS) {
    if (raw != btnStable_) {
      btnStable_ = raw;
      if (btnStable_) {
        // pressed
        btnWasPressed_   = true;
        btnPressStartMs_ = millis();
      } else {
        // released
        if (btnWasPressed_) {
          ev.click = true;
          if ((millis() - btnPressStartMs_) >= LONG_MS)
            ev.longPress = true;
        }
        btnWasPressed_ = false;
      }
    }
  }

  return ev;
}
