#pragma once
#include "MenuConfig.h"

void setup();
void loop();
void IRAM_ATTR readEncoderISR();
void StartReflowProfile(ReflowProfile& profile);
