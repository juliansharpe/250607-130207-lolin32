#include "SolderProfile.h"
#include "ArduinoMenu.h"

// Example profile values, adjust as needed
#define PHASE_MS(x) ((x) * 1000)

SolderProfile::SolderProfile()
    : phases{
        Phase( 25, 150, PHASE_MS(120), PHASE_MS(120)),   // Preheat
        Phase(150, 180, PHASE_MS( 60), PHASE_MS(120)),   // Soak
        Phase(180, 220, PHASE_MS( 30), PHASE_MS( 60)),   // Peak
        Phase(220, 200, PHASE_MS( 20), PHASE_MS( 20)),   // Dwell (endTemp can be lower for cooldown)
        Phase(200,   0, PHASE_MS( 60), PHASE_MS( 60))    // Cool
      },
      phaseIdx(PREHEAT)
{}

void SolderProfile::begin() {
    phaseIdx = PREHEAT;
    for (int i = 0; i < 5; ++i) {
        phases[i].achievedTemp = 0;
        phases[i].startTimeMs = 0;
        phases[i].completed = false;
    }
}

void SolderProfile::update(float actualTemp, uint32_t nowMs) {
    if (phaseIdx == COMPLETE) return;
    Phase& phase = phases[phaseIdx];

    if (phase.startTimeMs == 0)
        phase.startTimeMs = nowMs;

    uint32_t elapsed = nowMs - phase.startTimeMs;

    bool tempReached = (actualTemp >= phase.achievedTemp);
    bool minTimeReached = (elapsed >= phase.minTimeMs);
    bool maxTimeExceeded = (elapsed >= phase.maxTimeMs);

    if ((tempReached && minTimeReached) || maxTimeExceeded) {
        phase.completed = true;
        nextPhase(nowMs);
    }
}

void SolderProfile::nextPhase(uint32_t nowMs) {
    if (phaseIdx < COOL) {
        phaseIdx = static_cast<PhaseType>(phaseIdx + 1);
        phases[phaseIdx].startTimeMs = nowMs;
    } else {
        phaseIdx = COMPLETE;
    }
}

SolderProfile::PhaseType SolderProfile::currentPhase() const {
    return phaseIdx;
}

bool SolderProfile::isComplete() const {
    return phaseIdx == COMPLETE;
}

void SolderProfile::drawGraph(TFT_eSPI& tft, int x, int y, int w, int h) {
    // Find total time and temp range
    uint32_t totalTime = 0;
    float minTemp = 0;
    float maxTemp = phases[0].startTemp;
    for (int i = 0; i < 5; ++i) {
        totalTime += phases[i].maxTimeMs;
        if (phases[i].startTemp < minTemp) minTemp = phases[i].startTemp;
        if (phases[i].endTemp < minTemp) minTemp = phases[i].endTemp;
        if (phases[i].startTemp > maxTemp) maxTemp = phases[i].startTemp;
        if (phases[i].endTemp > maxTemp) maxTemp = phases[i].endTemp;
    }
    if (maxTemp == minTemp) maxTemp += 1; // avoid div by zero

    // Draw axes
    tft.drawRect(x, y, w, h, TFT_NAVY);

    // Draw horizontal grid lines and temperature labels (every 50 deg)
    int tempStep = 50;
    for (int temp = ((int)minTemp / tempStep) * tempStep; temp <= (int)maxTemp; temp += tempStep) {
        int py = y + h - (int)((temp - minTemp) * h / (maxTemp - minTemp));
        if( (temp % 100) == 0) { // every 100 degrees
            tft.drawFastHLine(x, py, w, TFT_NAVY);
        } else {
            tft.drawFastHLine(x, py, w, 0x0008);
        }
        tft.setTextColor(TFT_NAVY, TFT_BLACK);
        if( ((temp % 100) == 0) and (temp > 0)) { // every 100 degrees
            tft.setTextSize(1);
            tft.setCursor(x+4, py-4);
            tft.printf("%3dc", temp);
        }
    }

    // Draw vertical grid lines for each 60s interval
    uint32_t secondsTotal = totalTime / 1000;
    for (uint32_t sec = 60; sec < secondsTotal; sec += 60) {
        int px = x + (int)((sec * 1000UL * w) / totalTime);
        tft.setTextColor(TFT_NAVY, TFT_BLACK);
        if( sec % 120 == 0) { // every 2 minutes
            tft.drawFastVLine(px, y, h, TFT_NAVY);
            tft.setTextSize(1);
            tft.setCursor(px - 10, y + h - 12);
            tft.printf("%lus", sec);
        } else {
            tft.drawFastVLine(px, y, h, 0x0008);
        }
    }

    // Draw profile line
    uint32_t elapsed = 0;
    int prevX = x, prevY = y + h - (int)((phases[0].startTemp - minTemp) * h / (maxTemp - minTemp));
    for (int i = 0; i < 5; ++i) {
        elapsed += phases[i].maxTimeMs;
        int px = x + (int)((elapsed * w) / totalTime);
        int py = y + h - (int)((phases[i].endTemp - minTemp) * h / (maxTemp - minTemp));
        tft.drawLine(prevX, prevY, px, py, TFT_RED);
        prevX = px;
        prevY = py;
    }

    // Optionally, draw phase labels
    // ...add label code if desired...
}

SolderProfile solderProfile; 