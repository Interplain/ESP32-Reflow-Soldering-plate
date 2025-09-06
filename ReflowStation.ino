// Fan_2Stage_Test_ESP32core3.ino
#include <Arduino.h>
#include "esp32-hal-ledc.h"

// ---- wiring ----
#define FAN_PIN       19      // GPIO19 -> 1k -> BC337 base
#define ACTIVE_LOW    0       // leave 0 for BC337 low-side (HIGH = ON)

// ---- PWM settings ----
#define FAN_PWM_FREQ  100
#define FAN_PWM_RES   12
#define PWM_MAX       ((1 << FAN_PWM_RES) - 1)

// helper to set ON/OFF respecting polarity
inline void fanDigital(bool on) {
  digitalWrite(FAN_PIN, (ACTIVE_LOW ? !on : on) ? HIGH : LOW);
}

// helper to set PWM duty (0..PWM_MAX) respecting polarity
inline void fanPwm(uint16_t duty) {
  if (duty > PWM_MAX) duty = PWM_MAX;
  if (ACTIVE_LOW) duty = PWM_MAX - duty;
  ledcWrite(FAN_PIN, duty);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n== Fan test ==");
  Serial.println("Phase 1: digital ON/OFF");
  pinMode(FAN_PIN, OUTPUT);
}

void loop() {
  // -------- Phase 1: digital ON/OFF for ~20s --------
  static uint32_t t0 = millis();
  if (millis() - t0 < 20000UL) {            // 20 seconds total
    static bool on = false;
    static uint32_t last = 0;
    if (millis() - last >= 3000) {          // toggle every 3s
      last = millis();
      on = !on;
      fanDigital(on);
      Serial.println(on ? "Fan ON (digital)" : "Fan OFF (digital)");
    }
    return;
  }

  // -------- Phase 2: PWM sweep forever --------
  static bool pwmAttached = false;
  if (!pwmAttached) {
    Serial.println("Phase 2: PWM sweep (100%, 50%, 25%, OFF, ramp)");
    // hand control to LEDC PWM (ESP32 core 3.x API)
    ledcAttach(FAN_PIN, FAN_PWM_FREQ, FAN_PWM_RES);
    fanPwm(0);
    pwmAttached = true;
  }

  // demo pattern
  fanPwm(PWM_MAX);   Serial.println("PWM 100%"); delay(2000);
  fanPwm(PWM_MAX/2); Serial.println("PWM 50%");  delay(2000);
  fanPwm(PWM_MAX/4); Serial.println("PWM 25%");  delay(2000);
  fanPwm(0);         Serial.println("PWM OFF");  delay(2000);

  // smooth ramp up/down
  Serial.println("PWM ramp up");
  for (int d = 0; d <= PWM_MAX; d += 128) { fanPwm(d); delay(60); }
  Serial.println("PWM ramp down");
  for (int d = PWM_MAX; d >= 0; d -= 128) { fanPwm(d); delay(60); }
}
