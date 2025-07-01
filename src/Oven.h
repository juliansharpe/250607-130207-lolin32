#pragma once

#include <TFT_eSPI.h>
#include <stdint.h>

class Oven {
public:
    struct DataPoint {
        uint16_t temp;
    };
    static const int MAX_POINTS = 12 * 60 * 3; // 12 hours max at one per 20 seconds
    static const unsigned long INC_MS = 20000UL; // 12 hours max at one per 10 seconds
    

    Oven();
    void initGraph(TFT_eSPI& tft, int x, int y, int w, int h);
    void updateGraph(float actualTemp, float setTemp);
    void reset();
    void setGraphLimits(float maxTemp, uint32_t maxTimeMins);
    void redrawGraph();

private:
    float defaultTemp;
    uint32_t maxTimeMs;
    float graphMaxTemp;
    uint32_t graphTotalTimeMins;
    uint32_t startTimeMs;
    TFT_eSPI* tftRef;
    int graphX, graphY, graphW, graphH;
    bool graphInitialized;
    // Data points for graph
    DataPoint points[MAX_POINTS];
    int numPoints;
    int32_t lastPointInc;
    void recordPoint(uint32_t nowMs, float temp);
    int tempToY(float temp) const;
    int timeMsToX(uint32_t elapsedMs) const;
    float currentSetpoint = 0.0f;
};
