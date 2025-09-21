#pragma once
#include <Arduino.h>

class SensorManager {
public:
  void begin(uint8_t thermFrontPin, uint8_t thermBackPin, float vref = 3.30f);
  void update();                          // call every loop

  float tempFront() const { return tF_; }
  float tempBack()  const { return tB_;  }

  // Calibration (offset in °C, optional scale)
  void setFrontCal(float offsetC, float scale = 1.0f);
  void setBackCal (float offsetC, float scale = 1.0f);

  // Quick one-point calibration at room temp
  void calibrateAtRoomTemp(float roomTempC = 23.0f);

private:
  // Pins
  uint8_t pF_ = 32, pB_ = 33;

  // Last filtered temps
  float   tF_ = 25.0f, tB_ = 25.0f;

  // Divider / thermistor constants (100k 3950 + 6.8k to GND)
  static constexpr float SERIES_RESISTOR      = 6800.0f;     // 6.8kΩ → GND
  static constexpr float THERMISTOR_NOMINAL   = 100000.0f;   // 100kΩ @ 25°C
  static constexpr float TEMPERATURE_NOMINAL  = 25.0f;       // °C
  static constexpr float BETA_COEFFICIENT     = 3950.0f;     // Beta
  static constexpr float ADC_MAX              = 4095.0f;     // 12-bit

  // Measured ADC reference (you can pass your measured 3.285 V in begin)
  float vref_ = 3.30f;

  // Per-channel calibration
  float   frontOffset_ = 0.0f, frontScale_ = 1.0f;
  float   backOffset_  = 0.0f, backScale_  = 1.0f;

  // Helpers
  float readThermC_(uint8_t pin, float lastStable);
  float adcToTemp_(float adc);
};
