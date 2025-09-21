// #include "SensorManager.h"

#include "SensorManager.h"
void SensorManager::setFrontCal(float offsetC, float scale) {
  frontOffset_ = offsetC;
  frontScale_ = scale;
}

void SensorManager::setBackCal(float offsetC, float scale) {
  backOffset_ = offsetC;
  backScale_ = scale;
}

void SensorManager::begin(uint8_t f, uint8_t b) {
  pF_ = f; 
  pB_ = b;
  pinMode(pF_, INPUT);
  pinMode(pB_, INPUT);
  
  // Set ADC attenuation for better range
  analogSetPinAttenuation(pF_, ADC_11db);
  analogSetPinAttenuation(pB_, ADC_11db);
  analogReadResolution(12);  // 0-4095 range
  
  // Initialize smoothing arrays
  for (int i = 0; i < SMOOTH_SAMPLES; i++) {
    adcHistory_Front_[i] = 0;
    adcHistory_Back_[i] = 0;
  }
  
  Serial.println("SensorManager: Initialized with 100K 3950 thermistor + 6.8K resistor");
  Serial.printf("Front pin: %d, Back pin: %d\n", pF_, pB_);
}

void SensorManager::calibrateAtRoomTemp(float roomTempC) {
  Serial.println("=== CALIBRATING AT ROOM TEMPERATURE ===");
  Serial.println("Taking baseline readings...");
  
  // Take several readings and average them
  long sumFront = 0, sumBack = 0;
  int samples = 20;
  
  for (int i = 0; i < samples; i++) {
    sumFront += analogRead(pF_);
    sumBack += analogRead(pB_);
    delay(10);
  }
  
  float avgFront = sumFront / (float)samples;
  float currentTemp = adcToTemp_(avgFront);
  float offset = roomTempC - currentTemp;
  
  frontOffset_ = offset;
  backOffset_ = offset;
  
  Serial.printf("Target room temp: %.1f°C\n", roomTempC);
  Serial.printf("Calculated temp: %.1f°C\n", currentTemp);
  Serial.printf("Applied offset: %.2f°C\n", offset);
  Serial.println("Calibration complete!");
}

float SensorManager::smoothADC_(float newValue, float* history) {
  // Add new value to circular buffer
  history[historyIndex_] = newValue;
  
  // Calculate average
  float sum = 0;
  int count = historyFull_ ? SMOOTH_SAMPLES : (historyIndex_ + 1);
  for (int i = 0; i < count; i++) {
    sum += history[i];
  }
  
  return sum / count;
}

float SensorManager::adcToTemp_(float adc) {
  // Convert ADC to voltage
  float voltage = (adc / ADC_MAX) * VREF;
  
  // Calculate thermistor resistance from voltage divider
  // Thermistor on low side (connected to GND)
  float resistance = SERIES_RESISTOR * voltage / (VREF - voltage);
    
  // Steinhart-Hart Beta equation
  // 1/T = 1/T0 + (1/B) * ln(R/R0)
  float steinhart = log(resistance / THERMISTOR_NOMINAL) / BETA_COEFFICIENT;
  steinhart += 1.0f / (TEMPERATURE_NOMINAL + 273.15f);
  float tempC = (1.0f / steinhart) - 273.15f;
    
  return tempC;
}

float SensorManager::readThermC_(uint8_t pin) {
  float sum = 0;
  for (int i = 0; i < 16; i++) {  // Changed from 4 to 16
    sum += analogRead(pin);
    delayMicroseconds(100);
  }
  float adc = sum / 16.0f;  // Changed from 4.0f to 16.0f
  
  // Convert ADC to temperature using proper thermistor equation
  float temp = adcToTemp_(adc);
  
  // Apply smoothing
  bool isFront = (pin == pF_);
  float* history = isFront ? adcHistory_Front_ : adcHistory_Back_;
  float smoothedT = smoothADC_(temp, history);
  
  static bool indexUpdated = false;
  if (!indexUpdated) {
    historyIndex_++;
    if (historyIndex_ >= SMOOTH_SAMPLES) {
      historyIndex_ = 0;
      historyFull_ = true;
    }
    indexUpdated = true;
  }
  if (pin == pB_) indexUpdated = false;
  
  // Apply individual sensor calibration
  if (isFront) {
    smoothedT = (smoothedT + frontOffset_) * frontScale_;
  } else {
    smoothedT = (smoothedT + backOffset_) * backScale_;
  }
  
  return smoothedT;
}

void SensorManager::update() {
  static bool init = false;
  
  float f = readThermC_(pF_);
  float b = readThermC_(pB_);
  
  if (!init) { 
    tF_ = f; 
    tB_ = b; 
    init = true; 
  }
  
  // Much heavier smoothing - slower response but more stable
  tF_ = 0.05f * f + 0.95f * tF_;  // Changed from 0.20/0.80
  tB_ = 0.05f * b + 0.95f * tB_;  // Changed from 0.12/0.88
}
