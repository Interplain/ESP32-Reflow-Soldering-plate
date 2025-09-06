#include "SensorManager.h"
#include <math.h>

// --- Try to use your original LUT if available ---
#if __has_include("thermProf.h")
  #include "thermProf.h"  // must define: float thermProf[];
  #ifndef LOWESTTHERM
    #define LOWESTTHERM 85
  #endif
  #ifndef LOWESTTEMP
    #define LOWESTTEMP 23
  #endif
  #define THERM_USE_LUT 1
#else
  #define THERM_USE_LUT 0
#endif

// Beta model constants (fallback)
static constexpr float R0    = 100000.0f; // 100k @25C
static constexpr float T0_K  = 298.15f;   // 25C in Kelvin
static constexpr float BETA  = 3950.0f;   // B25/85
static constexpr float RFIX  = 100000.0f; // divider resistor

void SensorManager::begin(uint8_t f, uint8_t b){
  pF_ = f; pB_ = b;
  pinMode(pF_, INPUT);
  pinMode(pB_, INPUT);
  // Works for ADC1 and ADC2 when Wi-Fi/BT are off (you said they’ll stay off)
  analogSetPinAttenuation(pF_, ADC_11db);
  analogSetPinAttenuation(pB_, ADC_11db);
}

static inline float adc_avg32(uint8_t pin){
  (void)analogRead(pin); // settle mux
  uint32_t acc = 0;
  for (int i=0;i<32;i++) acc += analogRead(pin);
  float c = acc/32.0f;
  if (c < 1)    c = 1;
  if (c > 4094) c = 4094;
  return c;
}

#if THERM_USE_LUT
// --- Your original mapping: ADC -> Celsius via thermProf[] ---
static inline float adcToCelsius_fromLUT(int adc){
  if (adc < LOWESTTHERM) return LOWESTTEMP;
  int idx = adc - LOWESTTHERM;
  int maxIdx = (int)(sizeof(thermProf)/sizeof(thermProf[0])) - 1;
  if (idx > maxIdx) idx = maxIdx;
  return thermProf[idx];
}
#else
// --- Fallback: robust autodetect for divider orientation (works either way) ---
static inline float ntcC_fromCounts(float counts){
  auto R_to_C = [](float r_ntc)->float {
    float invT = (1.0f/T0_K) + (1.0f/BETA) * logf(r_ntc / R0);
    return (1.0f/invT) - 273.15f;
  };
  // Case A: RFIX->3V3, NTC->GND
  float r_down = RFIX * counts / (4095.0f - counts);
  float t_down = R_to_C(r_down);
  // Case B: NTC->3V3, RFIX->GND
  float r_up   = RFIX * (4095.0f - counts) / counts;
  float t_up   = R_to_C(r_up);
  auto plausible = [](float T){ return T > -20.0f && T < 250.0f; };
  if (plausible(t_up) && !plausible(t_down)) return t_up;
  if (plausible(t_down) && !plausible(t_up)) return t_down;
  // Pick the one nearer room temp if both look plausible
  return (fabsf(t_up - 25.0f) < fabsf(t_down - 25.0f)) ? t_up : t_down;
}
#endif

float SensorManager::readThermC_(uint8_t pin){
  float counts = adc_avg32(pin);
#if THERM_USE_LUT
  // Your LUT expects an integer ADC code
  int adc = (int)(counts + 0.5f);
  float T = adcToCelsius_fromLUT(adc);
#else
  float T = ntcC_fromCounts(counts);
#endif
  // 👇 Serial output:
  Serial.printf("ADC pin %d counts=%.0f  T=%.1f C\n", pin, counts, T);
  return T;
}

void SensorManager::update(){
  static bool init=false;
  float f = readThermC_(pF_);
  float b = readThermC_(pB_);
  if (!init){ tF_=f; tB_=b; init=true; }
  // match your original: front faster, back heavier smoothing (approx)
  tF_ = 0.20f * f + 0.80f * tF_;
  tB_ = 0.12f * b + 0.88f * tB_;
}
