#include "SensorManager.h"

void SensorManager::setFrontCal(float offsetC, float scale) {
  frontOffset_ = offsetC; frontScale_ = scale;
}

void SensorManager::setBackCal(float offsetC, float scale) {
  backOffset_ = offsetC;  backScale_  = scale;
}

void SensorManager::begin(uint8_t f, uint8_t b, float vref) {
  pF_ = f;  pB_ = b;  vref_ = vref;

  pinMode(pF_, INPUT);
  pinMode(pB_, INPUT);

  // 3.3V range (needed because at high temp the node approaches Vref)
  analogSetPinAttenuation(pF_, ADC_11db);
  analogSetPinAttenuation(pB_, ADC_11db);
  analogReadResolution(12); // 0..4095

  Serial.println("SensorManager: 100k/3950 NTC with 6.8k pull-DOWN (to GND)");
  Serial.printf("Front pin=%d  Back pin=%d  Vref=%.3fV\n", pF_, pB_, vref_);
}

/**
 * Correct divider for YOUR wiring:
 * 3V3 ── NTC ──●── 6.8k ── GND, ADC at ●
 * Vnode = Vref * (Rseries / (Rseries + Rntc))
 * => Rntc = Rseries * (Vref - Vnode) / Vnode
 */
float SensorManager::adcToTemp_(float adc) {
  // ADC → Voltage
  float v = (adc / ADC_MAX) * vref_;
  // Avoid divide-by-zero / log(<=0)
  if (v < 0.0005f) v = 0.0005f;
  if (v > vref_ - 0.0005f) v = vref_ - 0.0005f;

  // Compute NTC resistance for pull-DOWN wiring (6.8k to GND)
  float rntc = SERIES_RESISTOR * (vref_ - v) / v;

  // Beta equation
  float invT = (1.0f / (TEMPERATURE_NOMINAL + 273.15f))
             + (log(rntc / THERMISTOR_NOMINAL) / BETA_COEFFICIENT);
  float tempC = (1.0f / invT) - 273.15f;
  return tempC;
}

// Oversample + clamp + apply per-channel calibration
float SensorManager::readThermC_(uint8_t pin, float lastStable) {
  // Oversample for noise reduction
  float sum = 0;
  for (int i = 0; i < 32; i++) {
    sum += analogRead(pin);
    delayMicroseconds(80);
  }
  float adc = sum / 32.0f;

  float t = adcToTemp_(adc);

  // Basic sanity clamps (hold last good if nonsense)
  if (!isfinite(t) || t < -40.0f || t > 350.0f) {
    t = lastStable;
  }

  // Apply per-sensor calibration
  if (pin == pF_) t = (t + frontOffset_) * frontScale_;
  else            t = (t + backOffset_)  * backScale_;

  return t;
}

void SensorManager::calibrateAtRoomTemp(float roomTempC) {
  Serial.println("=== Room-temp calibration ===");
  long sumF = 0, sumB = 0;
  const int N = 32;
  for (int i = 0; i < N; i++) {
    sumF += analogRead(pF_);
    sumB += analogRead(pB_);
    delay(10);
  }
  float avgF = sumF / float(N);
  float avgB = sumB / float(N);

  float tF_now = adcToTemp_(avgF);
  float tB_now = adcToTemp_(avgB);

  frontOffset_ = roomTempC - tF_now;
  backOffset_  = roomTempC - tB_now;

  Serial.printf("Target %.1fC  Front raw %.1fC  Back raw %.1fC\n",
                roomTempC, tF_now, tB_now);
  Serial.printf("Applied offsets: front %.2fC  back %.2fC\n",
                frontOffset_, backOffset_);
}

void SensorManager::update() {
  static bool init = false;

  // Read instantaneous temps (use last filtered as fallback if needed)
  float f = readThermC_(pF_, tF_);
  float b = readThermC_(pB_, tB_);

  if (!init) { tF_ = f; tB_ = b; init = true; }

  // EMA smoothing — responsive but stable for reflow ramps
  const float alpha = 0.15f;           // ~7-sample time constant
  tF_ = alpha * f + (1 - alpha) * tF_;
  tB_ = alpha * b + (1 - alpha) * tB_;
}
