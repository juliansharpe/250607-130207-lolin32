#pragma once

#include <Arduino.h>

class ElementPWM {
public:
    ElementPWM(uint8_t mainPin, uint8_t fryPin, uint32_t pwmPeriodMs = 1000);
    void setPWM(uint8_t mainPWM, uint8_t fryPWM);
    void process();

private:
    uint8_t _mainPin;
    uint8_t _fryPin;
    uint8_t _mainPWM;
    uint8_t _fryPWM;
    uint8_t _mainPWMSet;
    uint8_t _fryPWMSet;
    uint32_t _pwmPeriodMs;
    uint32_t _cycleStartMs;
    void updateOutputs(uint32_t delta);
};
