#pragma once

#include <TFT_eSPI.h>
#include <stdint.h>

class Oven {
public:
    struct DataPoint {
        float temp;
    };
    static const int MAX_POINTS = 120; // 2 hours max, 1 per minute

    Oven(float defaultTemp, uint32_t maxTimeMs, float maxGraphTemp = 0, uint32_t maxGraphTimeMs = 0);
    void initGraph(TFT_eSPI& tft, int x, int y, int w, int h);
    void updateGraph(float actualTemp);
    void reset();
    void setGraphLimits(float maxTemp, uint32_t maxTimeMins);
    void redrawGraph();

private:
    float defaultTemp;
    uint32_t maxTimeMs;
    float graphMaxTemp;
    uint32_t graphTotalTime;
    uint32_t startTimeMs;
    TFT_eSPI* tftRef;
    int graphX, graphY, graphW, graphH;
    float graphMinTemp;
    bool graphInitialized;
    // Data points for graph
    DataPoint points[MAX_POINTS];
    int numPoints;
    uint32_t lastPointMinute;
    void recordPoint(uint32_t nowMs, float temp);
};
