#include "Oven.h"
#include <Arduino.h>

Oven::Oven(float defaultTemp, uint32_t maxTimeMs, float maxGraphTemp, uint32_t maxGraphTimeMs)
    : defaultTemp(defaultTemp), maxTimeMs(maxTimeMs), startTimeMs(0), tftRef(nullptr),
      graphX(0), graphY(0), graphW(0), graphH(0),
      graphMinTemp(0), graphMaxTemp(maxGraphTemp), graphTotalTime(maxGraphTimeMs), graphInitialized(false)
{}

void Oven::initGraph(TFT_eSPI& tft, int x, int y, int w, int h) {
    tftRef = &tft;
    graphX = x;
    graphY = y;
    graphW = w;
    graphH = h;
    graphMinTemp = 0;
    graphMaxTemp = defaultTemp + 25;
    graphTotalTime = maxTimeMs + 60000; // Add 60s for margin
    if (graphMaxTemp == graphMinTemp) graphMaxTemp += 1;
    tft.drawRect(graphX, graphY, graphW, graphH, TFT_NAVY);
    // Draw horizontal grid lines and temperature labels (every 50 deg)
    int tempStep = 50;
    for (int temp = ((int)graphMinTemp / tempStep) * tempStep; temp <= (int)graphMaxTemp; temp += tempStep) {
        int py = graphY + graphH - (int)((temp - graphMinTemp) * graphH / (graphMaxTemp - graphMinTemp));
        if ((temp % 100) == 0) {
            tft.drawFastHLine(graphX, py, graphW, TFT_NAVY);
        } else {
            tft.drawFastHLine(graphX, py, graphW, 0x0008);
        }
        tft.setTextColor(TFT_NAVY, TFT_BLACK);
        if (((temp % 100) == 0) && (temp > 0)) {
            tft.setTextSize(1);
            tft.setCursor(graphX+4, py-4);
            tft.printf("%3dc", temp);
        }
    }
    // Draw vertical grid lines for each 60s interval
    uint32_t secondsTotal = graphTotalTime / 1000;
    for (uint32_t sec = 60; sec < secondsTotal; sec += 600) {
        int px = graphX + (int)((sec * 1000UL * graphW) / graphTotalTime);
        tft.setTextColor(TFT_NAVY, TFT_BLACK);
        if (sec % 120 == 0) {
            tft.drawFastVLine(px, graphY, graphH, TFT_NAVY);
            tft.setTextSize(1);
            tft.setCursor(px - 10, graphY + graphH - 12);
            tft.printf("%lum", sec/60);
        } else {
            tft.drawFastVLine(px, graphY, graphH, 0x0008);
        }
    }
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
    int px = graphX + (int)(((nowMs - startTimeMs) * graphW) / graphTotalTime);
    int py = graphY + graphH - (int)((actualTemp - graphMinTemp) * graphH / (graphMaxTemp - graphMinTemp));
    if (px >= graphX && px < graphX + graphW && py >= graphY && py < graphY + graphH) {
        tftRef->drawPixel(px, py, TFT_YELLOW);
    }
}

void Oven::reset() {
    startTimeMs = 0;
}
