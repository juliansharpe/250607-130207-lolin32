#pragma once

#include <TFT_eSPI.h>
#include <stdint.h>

class Oven {
public:
    Oven(float defaultTemp, uint32_t maxTimeMs);
    void initGraph(TFT_eSPI& tft, int x, int y, int w, int h);
    void updateGraph(float actualTemp);
    void reset();

private:
    float defaultTemp;
    uint32_t maxTimeMs;
    uint32_t startTimeMs;
    TFT_eSPI* tftRef;
    int graphX, graphY, graphW, graphH;
    float graphMinTemp, graphMaxTemp;
    uint32_t graphTotalTime;
    bool graphInitialized;
};
