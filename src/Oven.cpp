#include "Oven.h"
#include <Arduino.h>

Oven::Oven()
    : defaultTemp(0), maxTimeMs(15000UL), startTimeMs(0), tftRef(nullptr),
      graphX(0), graphY(0), graphW(0), graphH(0),
      graphMaxTemp(100), graphTotalTimeMins(15), graphInitialized(false)
{}

void Oven::initGraph(TFT_eSPI& tft, int x, int y, int w, int h, float maxTemp, uint32_t totalTimeMins) {
    tftRef = &tft;
    graphX = x;
    graphY = y;
    graphW = w;
    graphH = h;
    if (graphMaxTemp == 0) graphMaxTemp += 1; // Use 0 instead of graphMinTemp

    startTimeMs = millis();
    lastPointInc = -1;
    numPoints = 0;

    graphInitialized = true;

    setGraphLimits(maxTemp, totalTimeMins);

    // reset();
    // redrawGraph();
}

void Oven::updateGraph(float actualTemp, float setTemp) {
    if (!tftRef || !graphInitialized) return;
    // Check if the actual temperature exceeds the current graph max temperature
    if( actualTemp > graphMaxTemp) {
        setGraphLimits(actualTemp + 20, graphTotalTimeMins);
        redrawGraph();
    }
    
    uint32_t nowMs = millis();
    currentSetpoint = setTemp;
    int32_t elapsedInc = (nowMs - startTimeMs) / INC_MS;

    // Check if the current time exceeds the max graph time, extend by 1 minute if so
    uint32_t elapsedMins = (nowMs - startTimeMs) / 60000UL;
    if (elapsedMins >= graphTotalTimeMins) {
        setGraphLimits(graphMaxTemp, graphTotalTimeMins + 1);
        redrawGraph();
    }

    if (elapsedInc > lastPointInc && numPoints < MAX_POINTS) {
        points[numPoints].temp = (uint16_t) (actualTemp);
        numPoints++;
        lastPointInc = elapsedInc;
    }
    // Draw the latest point
    int px = timeMsToX(nowMs - startTimeMs);
    int py = tempToY(actualTemp);
    tftRef->drawPixel(px, py, TFT_YELLOW);

}

void Oven::reset() {
    startTimeMs = millis();
    lastPointInc = -1;
    numPoints = 0;
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
        int py = tempToY(temp);
        if ((temp % majorStep) == 0) {
            tftRef->drawFastHLine(graphX, py, graphW, TFT_NAVY);
            tftRef->setTextSize(1);
            tftRef->setCursor(graphX+3, py+ 3);
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

    if( totalTimeMins <= 15 ) {
        minorStep = 1; 
        majorStep = 5;  
    } else if (totalTimeMins < 60 ) {
        minorStep = 5; 
        majorStep = 10; 
    } else if (totalTimeMins <= 120 ) {
        minorStep = 10; 
        majorStep = 30; 
    } else if (totalTimeMins < (5*60)) {
        minorStep = 30; 
        majorStep = 60; 
    } else {
        minorStep = 60; 
        majorStep = 180; 
    }
    
    // Draw vertical lines for each minute
    for (uint32_t min = minorStep; min <= totalTimeMins; min += minorStep) {
        int px = timeMsToX(min * 60000UL);
        tftRef->setTextColor(TFT_NAVY, TFT_BLACK);
        if( min % majorStep == 0) {
            tftRef->drawFastVLine(px, graphY, graphH, TFT_NAVY);
            tftRef->setTextSize(1);
            tftRef->setCursor(px - 5, graphY + graphH - 12);
            char textBuffer[10];
            tftRef->setTextDatum(BR_DATUM);         
            if( min % 60 == 0) {
                snprintf(textBuffer, sizeof(textBuffer), "%luh", min/60);
            } else {
                snprintf(textBuffer, sizeof(textBuffer), "%lu", min % 60);
            }
            tftRef->drawString(textBuffer, px-1, graphY + graphH - 3);
            tftRef->setTextDatum(TL_DATUM);
        } else {
            tftRef->drawFastVLine(px, graphY, graphH, 0x0008);
        }
    }

    // Draw setpoint as a dark red horizontal line
    int setpointY = tempToY(currentSetpoint);
    if (setpointY >= graphY && setpointY < graphY + graphH) {
        tftRef->drawFastHLine(graphX, setpointY, graphW, 0x8000); // dark red
    }

    // Redraw all recorded points
    for (int i = 0; i < numPoints; ++i) {
        uint32_t pointTimeMs = i * INC_MS;
        int px = timeMsToX(pointTimeMs);
        int py = tempToY(points[i].temp);
        if( i < (numPoints-1) ) {
            // Draw the line
            int nextPx = timeMsToX(pointTimeMs + INC_MS);
            int nextPy = tempToY((float)points[i+1].temp);
            tftRef->drawLine(px, py, nextPx, nextPy, TFT_YELLOW);
        } else {
            int nextPx = timeMsToX(millis() - startTimeMs);
            int nextPy = tempToY((float)points[i].temp);
            tftRef->drawLine(px, py, nextPx, nextPy, TFT_YELLOW);
        }
    }
}

void Oven::setGraphLimits(float maxTemp, uint32_t maxTimeMins) {
    // Find the largest DataPoint temp
    float maxDataTemp = maxTemp;
    for (int i = 0; i < numPoints; ++i) {
        if (points[i].temp > maxDataTemp) {
            maxDataTemp = points[i].temp;
        }
    }
    graphMaxTemp = maxDataTemp;
    if (graphMaxTemp < maxTemp) graphMaxTemp = maxTemp;
    graphTotalTimeMins = maxTimeMins; 
    if (graphMaxTemp == 0) graphMaxTemp += 1; // Use 0 instead of graphMinTemp
    redrawGraph();
}

// Helper: Convert temperature to Y pixel value
int Oven::tempToY(float temp) const {
    if (graphMaxTemp == 0) return graphY + graphH;
    return graphY + graphH - (int)((temp) * graphH / (graphMaxTemp));
}

// Helper: Convert elapsed time in ms to X pixel value
int Oven::timeMsToX(uint32_t elapsedMs) const {
    if (graphTotalTimeMins == 0) return graphX;
    return graphX + (int)(((elapsedMs/1000UL) * graphW) / (graphTotalTimeMins * 60UL));
}