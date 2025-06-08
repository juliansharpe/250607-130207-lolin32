#include "SolderProfile.h"
#include "ArduinoMenu.h"

// Example profile values, adjust as needed
#define PHASE_MS(x) ((x) * 1000)

SolderProfile::SolderProfile()
    : phases{
        Phase("Preheat", 25, 150, PHASE_MS(120), PHASE_MS(120)),
        Phase("Soak",   150, 180, PHASE_MS( 60), PHASE_MS(120)),
        Phase("Peak",   180, 220, PHASE_MS( 30), PHASE_MS( 60)),
        Phase("Dwell",  220, 200, PHASE_MS( 20), PHASE_MS( 20)),
        Phase("Cool",   200,   0, PHASE_MS( 60), PHASE_MS( 60))
      },
      phaseIdx(PREHEAT),
      tftRef(nullptr),
      graphX(0), graphY(0), graphW(0), graphH(0),
      graphMinTemp(0), graphMaxTemp(phases[0].startTemp), graphTotalTime(0)
{}

void SolderProfile::begin() {
    phaseIdx = PREHEAT;
    for (int i = 0; i < 5; ++i) {
        phases[i].achievedTemp = 0;
        phases[i].startTimeMs = 0;
        phases[i].completed = false;
    }
}

void SolderProfile::initGraph(TFT_eSPI& tft, int x, int y, int w, int h) {
    tftRef = &tft;
    graphX = x;
    graphY = y;
    graphW = w;
    graphH = h;

    // Calculate and store min/max temp and total time for scaling
    graphMinTemp = 0;
    graphMaxTemp = phases[0].startTemp;
    graphTotalTime = 0;
    for (int i = 0; i < 5; ++i) {
        graphTotalTime += phases[i].minTimeMs;
        if (phases[i].startTemp < graphMinTemp) graphMinTemp = phases[i].startTemp;
        if (phases[i].endTemp < graphMinTemp) graphMinTemp = phases[i].endTemp;
        if (phases[i].startTemp > graphMaxTemp) graphMaxTemp = phases[i].startTemp;
        if (phases[i].endTemp > graphMaxTemp) graphMaxTemp = phases[i].endTemp;
    }
    if (graphMaxTemp == graphMinTemp) graphMaxTemp += 1;
}

void SolderProfile::update(float actualTemp, uint32_t nowMs) {
    if (phaseIdx == COMPLETE) return;
    Phase& phase = phases[phaseIdx];

    if (phase.startTimeMs == 0)
        phase.startTimeMs = nowMs;

    uint32_t elapsed = nowMs - phase.startTimeMs;

    // --- Plot actual temperature on the graph ---
    if (tftRef && graphW > 0 && graphH > 0) {
        uint32_t totalTime = graphTotalTime;
        float minTemp = graphMinTemp;
        float maxTemp = graphMaxTemp;

        uint32_t profileElapsed = 0;
        for (int i = 0; i < phaseIdx; ++i) {
            profileElapsed += phases[i].minTimeMs;
        }
        profileElapsed += elapsed;

        int px = graphX + (int)((profileElapsed * graphW) / totalTime);
        int py = graphY + graphH - (int)((actualTemp - minTemp) * graphH / (maxTemp - minTemp));
        if (px >= graphX && px < graphX + graphW && py >= graphY && py < graphY + graphH) {
            tftRef->drawPixel(px, py, TFT_YELLOW);
        }
    }
    // --- End plot actual temp ---

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

// Must call initGraph before drawGraph
void SolderProfile::drawGraph() {
    if (!tftRef || graphW <= 0 || graphH <= 0) return;
    TFT_eSPI& tft = *tftRef;
    uint32_t totalTime = graphTotalTime;
    float minTemp = graphMinTemp;
    float maxTemp = graphMaxTemp;

    // Draw axes
    tft.drawRect(graphX, graphY, graphW, graphH, TFT_NAVY);

    // Draw horizontal grid lines and temperature labels (every 50 deg)
    int tempStep = 50;
    for (int temp = ((int)minTemp / tempStep) * tempStep; temp <= (int)maxTemp; temp += tempStep) {
        int py = graphY + graphH - (int)((temp - minTemp) * graphH / (maxTemp - minTemp));
        if( (temp % 100) == 0) { // every 100 degrees
            tft.drawFastHLine(graphX, py, graphW, TFT_NAVY);
        } else {
            tft.drawFastHLine(graphX, py, graphW, 0x0008);
        }
        tft.setTextColor(TFT_NAVY, TFT_BLACK);
        if( ((temp % 100) == 0) and (temp > 0)) { // every 100 degrees
            tft.setTextSize(1);
            tft.setCursor(graphX+4, py-4);
            tft.printf("%3dc", temp);
        }
    }

    // Draw vertical grid lines for each 60s interval
    uint32_t secondsTotal = totalTime / 1000;
    for (uint32_t sec = 60; sec < secondsTotal; sec += 60) {
        int px = graphX + (int)((sec * 1000UL * graphW) / totalTime);
        tft.setTextColor(TFT_NAVY, TFT_BLACK);
        if( sec % 120 == 0) { // every 2 minutes
            tft.drawFastVLine(px, graphY, graphH, TFT_NAVY);
            tft.setTextSize(1);
            tft.setCursor(px - 10, graphY + graphH - 12);
            tft.printf("%lus", sec);
        } else {
            tft.drawFastVLine(px, graphY, graphH, 0x0008);
        }
    }

    // Draw profile line
    uint32_t elapsed = 0;
    int prevX = graphX, prevY = graphY + graphH - (int)((phases[0].startTemp - minTemp) * graphH / (maxTemp - minTemp));
    for (int i = 0; i < 5; ++i) {
        elapsed += phases[i].minTimeMs;
        int px = graphX + (int)((elapsed * graphW) / totalTime);
        int py = graphY + graphH - (int)((phases[i].endTemp - minTemp) * graphH / (maxTemp - minTemp));
        tft.drawLine(prevX, prevY, px, py, TFT_RED);
        prevX = px;
        prevY = py;
    }

    // Optionally, draw phase labels
    // ...add label code if desired...
}

float SolderProfile::idealTempAt(uint32_t ms) {
    uint32_t phaseStart = 0;
    for (int i = 0; i < 5; ++i) {
        uint32_t phaseEnd = phaseStart + phases[i].minTimeMs;
        if (ms <= phaseEnd) {
            float t = (float)(ms - phaseStart) / (float)(phases[i].minTimeMs);
            // Clamp t between 0 and 1
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            return phases[i].startTemp + t * (phases[i].endTemp - phases[i].startTemp);
        }
        phaseStart = phaseEnd;
    }
    // If time exceeds profile, return last phase's end temp
    return phases[4].endTemp;
}

SolderProfile solderProfile;