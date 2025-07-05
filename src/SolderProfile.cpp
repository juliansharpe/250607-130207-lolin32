#include "SolderProfile.h"
#include "ArduinoMenu.h"

#define PHASE_MS(x) ((x) * 1000)

static const SolderProfileParams defaultProfile = {
    {
        {"Preheat",   0, 150, PHASE_MS(180), PHASE_MS(180), false},
        {"Soak",    150, 180, PHASE_MS(120), PHASE_MS(120), false},
        {"Peak",    180, 235, PHASE_MS( 70), PHASE_MS(120), false},
        {"Dwell",   235, 235, PHASE_MS( 30), PHASE_MS( 20), false},
        {"Cool",    220,   0, PHASE_MS( 90), PHASE_MS( 90), false}
    },
    5
};

SolderProfile::SolderProfile()
    : SolderProfile(defaultProfile)
{}

SolderProfile::SolderProfile(const SolderProfileParams& params)
    : tftRef(nullptr),
      graphX(0), graphY(0), graphW(0), graphH(0),
      graphMinTemp(0), graphMaxTemp(0), graphTotalTime(0),
      phaseIdx(PREHEAT),
      numPhases(0)
{
    setProfile(params);
}

void SolderProfile::setProfile(const SolderProfileParams& params) {
    numPhases = params.numPhases;
    if(numPhases > SOLDER_PROFILE_MAX_PHASES) numPhases = SOLDER_PROFILE_MAX_PHASES;
    for(uint8_t i = 0; i < numPhases; ++i) {
        const auto& p = params.phases[i];
        phases[i] = Phase(p.phaseName, p.startTemp, p.endTemp, p.minTimeMs, p.maxTimeMs, p.maxRate);
    }
    phaseIdx = PREHEAT;
    // Optionally, reset graph/reflow state here if needed
}

void SolderProfile::startReflow() {
    reflowStartTime = millis();
    phaseIdx = PREHEAT;
    for (uint8_t i = 0; i < numPhases; ++i) {
        phases[i].startTimeMs = 0;
        phases[i].completed = false;
    }
    if(numPhases > 0)
        phases[0].startTimeMs = reflowStartTime;
}

void SolderProfile::initGraph(TFT_eSPI& tft, int x, int y, int w, int h) {
    tftRef = &tft;
    graphX = x;
    graphY = y;
    graphW = w;
    graphH = h;

    // Calculate and store min/max temp and total time for scaling
    graphMinTemp = 0;
    graphMaxTemp = (numPhases > 0) ? phases[0].startTemp : 0;
    graphTotalTime = 0;
    for (uint8_t i = 0; i < numPhases; ++i) {
        graphTotalTime += phases[i].minTimeMs;
        if (phases[i].startTemp < graphMinTemp) graphMinTemp = phases[i].startTemp;
        if (phases[i].endTemp < graphMinTemp) graphMinTemp = phases[i].endTemp;
        if (phases[i].startTemp > graphMaxTemp) graphMaxTemp = phases[i].startTemp;
        if (phases[i].endTemp > graphMaxTemp) graphMaxTemp = phases[i].endTemp;
    }
    graphTotalTime += 60000;
    graphMaxTemp += 25;
    if (graphMaxTemp == graphMinTemp) graphMaxTemp += 1;
}

void SolderProfile::update(float actualTemp, float output) {
    unsigned long nowMs = millis();
    if (phaseIdx == COMPLETE) return;
    if (phaseIdx >= numPhases) {
        phaseIdx = COMPLETE;
        return;
    }
    Phase& phase = phases[phaseIdx];

    if (phase.startTimeMs == 0)
        phase.startTimeMs = nowMs;

    uint32_t elapsed = nowMs - phase.startTimeMs;

    // --- Plot actual temperature on the graph ---
    if (tftRef && graphW > 0 && graphH > 0) {
        uint32_t totalTime = graphTotalTime;
        float minTemp = graphMinTemp;
        float maxTemp = graphMaxTemp;

        // Calculate elapsed time since reflow started
        uint32_t profileElapsed = nowMs - reflowStartTime;
        
        int px = graphX + (int)((profileElapsed * graphW) / totalTime);
        int py = graphY + graphH - (int)((actualTemp - minTemp) * graphH / (maxTemp - minTemp));
        if (px >= graphX && px < graphX + graphW && py >= graphY && py < graphY + graphH) {
            tftRef->drawPixel(px, py, TFT_YELLOW);
            if( elapsed < 100) {
                tftRef->drawRect(px-2,py-2, 5,5, TFT_YELLOW);
            }
        }

        // Draw a line for the output value
        tftRef->drawPixel(px, (graphY + graphH) - (output/4), TFT_DARKGREEN);

        // Print phase and actual temperature at the top of the screen
        TFT_eSPI& tft = *tftRef;
        tft.setTextColor(TFT_NAVY, TFT_BLACK);
        tft.setTextSize(1);
        tft.setCursor(0, 0);
        //tft.printf("%s T:%.0fC      ", phase.phaseName, actualTemp);

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
    if (phaseIdx + 1 < numPhases) {
        phaseIdx = static_cast<PhaseType>(phaseIdx + 1);
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
        if( (temp % 100) == 0) {
            tft.drawFastHLine(graphX, py, graphW, TFT_NAVY);
        } else {
            tft.drawFastHLine(graphX, py, graphW, 0x0008);
        }
        tft.setTextColor(TFT_NAVY, TFT_BLACK);
        if( ((temp % 100) == 0) and (temp > 0)) {
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
        if( sec % 120 == 0) {
            tft.drawFastVLine(px, graphY, graphH, TFT_NAVY);
            tft.setTextSize(1);
            tft.setCursor(px - 10, graphY + graphH - 12);
            tft.printf("%lus", sec);
        } else {
            tft.drawFastVLine(px, graphY, graphH, 0x0008);
        }
    }

    // Draw profile line and phase change circles
    uint32_t elapsed = 0;
    int prevX = graphX, prevY = graphY + graphH - (int)(((numPhases > 0 ? phases[0].startTemp : 0) - minTemp) * graphH / (maxTemp - minTemp));
    for (uint8_t i = 0; i < numPhases; ++i) {
        elapsed += phases[i].minTimeMs;
        int px = graphX + (int)((elapsed * graphW) / totalTime);
        int py = graphY + graphH - (int)((phases[i].endTemp - minTemp) * graphH / (maxTemp - minTemp));
        tft.drawLine(prevX, prevY, px, py, TFT_RED);

        if(i < numPhases-1) {
            tftRef->drawRect(px-1,py-1, 3,3, TFT_RED);
        }   
        prevX = px;
        prevY = py;
    }
}

float SolderProfile::getIdealTemp() {
    uint32_t nowMs = millis();
    uint32_t elapsed = nowMs - reflowStartTime;
    uint32_t phaseStart = 0;
    for (uint8_t i = 0; i < numPhases; ++i) {
        uint32_t phaseEnd = phaseStart + phases[i].minTimeMs;
        if (elapsed <= phaseEnd) {
            float t = (float)(elapsed - phaseStart) / (float)(phases[i].minTimeMs);
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            return phases[i].startTemp + t * (phases[i].endTemp - phases[i].startTemp);
        }
        phaseStart = phaseEnd;
    }
    return (numPhases > 0) ? phases[numPhases-1].endTemp : 0;
}

float SolderProfile::getSetpoint() {
    if (phaseIdx == COMPLETE || numPhases == 0) return (numPhases > 0) ? phases[numPhases-1].endTemp : 0;
    Phase& phase = phases[phaseIdx];
    if (phase.maxRate) {
        return phase.endTemp;
    } else {
        return getIdealTemp();
    }
}

float SolderProfile::getFeedForwardSlope(uint32_t deltaMs) {
    uint32_t nowMs = millis();
    uint32_t elapsed = nowMs - reflowStartTime + deltaMs;
    uint32_t phaseStart = 0;
    int phaseIdxAtTime = -1;

    for (uint8_t i = 0; i < numPhases; ++i) {
        uint32_t phaseEnd = phaseStart + phases[i].minTimeMs;
        if (elapsed < phaseEnd) {
            phaseIdxAtTime = i;
            break;
        }
        phaseStart = phaseEnd;
    }

    if (phaseIdxAtTime == -1) {
        phaseIdxAtTime = (numPhases > 0) ? numPhases-1 : 0;
    }

    if (phaseIdxAtTime+1 < numPhases) {
        uint32_t phaseEnd = 0;
        for (int i = 0; i <= phaseIdxAtTime; ++i) {
            phaseEnd += phases[i].minTimeMs;
        }
        if (elapsed == phaseEnd && phaseIdxAtTime+1 < numPhases) {
            phaseIdxAtTime++;
        }
    }

    const Phase& phase = phases[phaseIdxAtTime];
    float slope = 0.0f;
    if (phase.minTimeMs > 0) {
        slope = (phase.endTemp - phase.startTemp) / ((float)phase.minTimeMs / 1000.0f);
    }
    return slope;
};

SolderProfile solderProfile;