#pragma once
#include <Arduino.h>

class SensorManager {
public:
  void setFrontCal(float offsetC, float scale=1.0f);
  void setBackCal (float offsetC, float scale=1.0f);
  void begin(uint8_t thermFrontPin, uint8_t thermBackPin);
  void update();                          // call every loop
  float tempFront() const { return tF_; }
  float tempBack()  const { return tB_; }

private:
  uint8_t pF_, pB_;
  float   tF_=25, tB_=25;
  float readThermC_(uint8_t pin);         // 100k B3950
};
