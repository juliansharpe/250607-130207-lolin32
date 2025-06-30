#include "Oven.h"
#include <Arduino.h>

Oven::Oven(float defaultTemp, uint32_t maxTimeMs, float maxGraphTemp, uint32_t maxGraphTimeMins)
    : defaultTemp(defaultTemp), maxTimeMs(maxTimeMs), startTimeMs(0), tftRef(nullptr),
      graphX(0), graphY(0), graphW(0), graphH(0),
      /* graphMinTemp(0), */ graphMaxTemp(maxGraphTemp), graphTotalTimeMins(maxGraphTimeMins), graphInitialized(false)
{}

void Oven::initGraph(TFT_eSPI& tft, int x, int y, int w, int h) {
    tftRef = &tft;
    graphX = x;
    graphY = y;
    graphW = w;
    graphH = h;
    // graphMinTemp = 0; // Removed
    graphMaxTemp = defaultTemp + 25;
    graphTotalTimeMins = 60; 
    if (graphMaxTemp == 0) graphMaxTemp += 1; // Use 0 instead of graphMinTemp
    reset();
    redrawGraph();
    graphInitialized = true;
}

void Oven::updateGraph(float actualTemp) {
    if (!tftRef || !graphInitialized) return;
    uint32_t nowMs = millis();
    if (startTimeMs == 0) {
        startTimeMs = nowMs;
        lastPointMinute = 0;
        numPoints = 0;
    }
    uint32_t elapsedMin = (nowMs - startTimeMs) / 60000UL;
    if (elapsedMin > lastPointMinute && numPoints < MAX_POINTS) {
        points[numPoints].temp = actualTemp;
        numPoints++;
        lastPointMinute = elapsedMin;
    }
    // Draw the latest point
    int px = graphX + (int)(((nowMs - startTimeMs) * graphW) / (graphTotalTimeMins * 60000UL));
    int py = graphY + graphH - (int)((actualTemp /* - 0 */) * graphH / (graphMaxTemp /* - 0 */));
    if (px >= graphX && px < graphX + graphW && py >= graphY && py < graphY + graphH) {
        tftRef->drawPixel(px, py, TFT_YELLOW);
    }
}

void Oven::reset() {
    startTimeMs = 0;
}

void Oven::redrawGraph() {
    if (!tftRef || !graphInitialized) return;
    // Clear graph area
    tftRef->fillRect(graphX, graphY, graphW, graphH, TFT_BLACK);
    tftRef->drawRect(graphX, graphY, graphW, graphH, TFT_NAVY);

    // Redraw grid lines (same as in initGraph)
    uint32_t minorStep, majorStep;
    if (graphMaxTemp >= 200 ) {
        minorStep = 50; 
        majorStep = 100; 
    } else if (graphMaxTemp >= 100) {
        minorStep = 25;  
        majorStep = 50; 
    } else {
        minorStep = 10; 
        majorStep = 50; 
    }
    
    for (uint32_t temp = minorStep; temp <= graphMaxTemp; temp += minorStep) {
        int py = graphY + graphH - (int)(temp * graphH / graphMaxTemp);
        if ((temp % majorStep) == 0) {
            tftRef->drawFastHLine(graphX, py, graphW, TFT_NAVY);
            tftRef->setTextSize(1);
            tftRef->setCursor(graphX+4, py-4);
            tftRef->setTextColor(TFT_NAVY, TFT_BLACK);
            tftRef->printf("%luc", temp);
        } else {
            tftRef->drawFastHLine(graphX, py, graphW, 0x0008);
        }
    }
    // Redraw vertical grid lines
    // Cacluate the time in mins to get approximately 10 vertical lines in the max graph time
    long totalTimeMins = graphTotalTimeMins;
    if (totalTimeMins == 0) {
        totalTimeMins = 1; 
    }

    if (totalTimeMins <= 120 ) {
        minorStep = 10; 
        majorStep = 30; 
    } else if (totalTimeMins < (5*60)) {
        minorStep = 30; 
        majorStep = 60; 
    } else {
        minorStep = 60; 
        majorStep = 180; 
    }
    
    for (uint32_t min = minorStep; min <= totalTimeMins; min += minorStep) {
        int px = graphX + (int)((min * graphW) / graphTotalTimeMins);
        tftRef->setTextColor(TFT_NAVY, TFT_BLACK);
        if( min % majorStep == 0) {
            tftRef->drawFastVLine(px, graphY, graphH, TFT_NAVY);
            if( min % 60 == 0) {
                tftRef->setTextSize(1);
                tftRef->setCursor(px - 5, graphY + graphH - 12);
                tftRef->printf("%luh", min/60);
            } 
        } else {
            tftRef->drawFastVLine(px, graphY, graphH, 0x0008);
        }
    }

    // Redraw all recorded points
    for (int i = 0; i < numPoints; ++i) {
        uint32_t pointTimeMin = i;
        int px = graphX + (int)((pointTimeMin * graphW) / graphTotalTimeMins);
        int py = graphY + graphH - (int)((points[i].temp /* - 0 */) * graphH / (graphMaxTemp /* - 0 */));
        if (px >= graphX && px < graphX + graphW && py >= graphY && py < graphY + graphH) {
            if( i < (numPoints-1) ) {
                // Draw the line
                int nextPx = graphX + (int)(((pointTimeMin + 1) * graphW) / graphTotalTimeMins);
                int nextPy = graphY + graphH - (int)(points[i+1].temp * graphH / graphMaxTemp);
                tftRef->drawLine(px, py, nextPx, nextPy, TFT_YELLOW);
            } 
        }
    }
}

void Oven::setGraphLimits(float maxTemp, uint32_t maxTimeMins) {
    graphMaxTemp = maxTemp;
    graphTotalTimeMins = maxTimeMins; 
    if (graphMaxTemp == 0) graphMaxTemp += 1; // Use 0 instead of graphMinTemp
    redrawGraph();
}