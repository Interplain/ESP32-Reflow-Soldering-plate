#pragma once
#include <Arduino.h>

class SensorManager {
public:
  void setFrontCal(float offsetC, float scale=1.0f);
  void setBackCal(float offsetC, float scale=1.0f);
  void begin(uint8_t thermFrontPin, uint8_t thermBackPin);
  void update();                          // call every loop
  float tempFront() const { return tF_; }
  float tempBack()  const { return tB_; }
  
  // Calibration methods
  void calibrateAtRoomTemp(float roomTempC = 23.0f);

private:
  uint8_t pF_, pB_;
  float   tF_=25, tB_=25;
  
  // Thermistor constants for 100K 3950 with 6.8K series resistor
  static constexpr float SERIES_RESISTOR = 6800.0f;      // 6.8K ohm
  static constexpr float THERMISTOR_NOMINAL = 100000.0f; // 100K at 25°C
  static constexpr float TEMPERATURE_NOMINAL = 25.0f;
  static constexpr float BETA_COEFFICIENT = 3950.0f;
  static constexpr float ADC_MAX = 4095.0f;
  static constexpr float VREF = 3.3f;
  
  // Calibration parameters
  float   frontOffset_=0, frontScale_=1.0f;
  float   backOffset_=0, backScale_=1.0f;
  
  // Smoothing for stability
  static constexpr int SMOOTH_SAMPLES = 10;
  float   adcHistory_Front_[SMOOTH_SAMPLES];
  float   adcHistory_Back_[SMOOTH_SAMPLES];
  int     historyIndex_ = 0;
  bool    historyFull_ = false;
  
  float readThermC_(uint8_t pin);
  float smoothADC_(float newValue, float* history);
  float adcToTemp_(float adc);
};
