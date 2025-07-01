#pragma once
#include "MenuConfig.h"

extern TFT_eSPI gfx;

void setup();
void loop();
void IRAM_ATTR readEncoderISR();
void StartReflowProfile(ReflowProfile& profile);
void StartOven();
