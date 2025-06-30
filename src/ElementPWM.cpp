#include "ElementPWM.h"

ElementPWM::ElementPWM(uint8_t mainPin, uint8_t fryPin, uint32_t pwmPeriodMs)
    : _mainPin(mainPin), _fryPin(fryPin), _pwmPeriodMs(pwmPeriodMs), _mainPWM(0), _fryPWM(0), _mainPWMSet(0), _fryPWMSet(0), _cycleStartMs(0)
{
    pinMode(_mainPin, OUTPUT);
    pinMode(_fryPin, OUTPUT);
    digitalWrite(_mainPin, LOW);
    digitalWrite(_fryPin, LOW);
    _cycleStartMs = millis();
}

void ElementPWM::setPWM(uint8_t mainPWM, uint8_t fryPWM)
{
    _mainPWM = constrain(mainPWM, 0, 100);
    _fryPWM = constrain(fryPWM, 0, 100);
}

void ElementPWM::process()
{
    uint32_t now = millis();
    uint32_t delta = (now - _cycleStartMs) * 100 / _pwmPeriodMs;
    if (delta > 100) delta = 100;

    // Start of new PWM cycle
    if ((now - _cycleStartMs) >= _pwmPeriodMs) {
        _mainPWMSet = _mainPWM;
        _fryPWMSet = _fryPWM;
        _cycleStartMs = now;
        digitalWrite(_mainPin, HIGH);
        digitalWrite(_fryPin, HIGH);
        delta = 0;
    }

    updateOutputs(delta);
}

void ElementPWM::updateOutputs(uint32_t delta)
{
    if (delta >= _mainPWMSet) {
        digitalWrite(_mainPin, LOW);
    }
    if (delta >= _fryPWMSet) {
        digitalWrite(_fryPin, LOW);
    }
}
