#pragma once

#include <stdint.h>
#include <TFT_eSPI.h>

class SolderProfile {
public:
    enum PhaseType { PREHEAT, SOAK, PEAK, DWELL, COOL, COMPLETE };

    struct Phase {
        float startTemp;
        float endTemp;
        float achievedTemp;
        uint32_t minTimeMs;
        uint32_t maxTimeMs;
        uint32_t startTimeMs;
        bool completed;

        Phase(float s, float e, float minT, float maxT)
            : startTemp(s), endTemp(e), achievedTemp(e*.95), minTimeMs(minT), maxTimeMs(maxT), startTimeMs(0), completed(false) {}
    };

    SolderProfile();
    void begin();
    void update(float actualTemp, uint32_t nowMs);
    PhaseType currentPhase() const;
    bool isComplete() const;
    void initGraph(TFT_eSPI& tft, int x, int y, int w, int h);
    void drawGraph();

    Phase phases[5];
private:
    PhaseType phaseIdx;
    void nextPhase(uint32_t nowMs);

    // --- Graph state ---
    TFT_eSPI* tftRef = nullptr;
    int graphX = 0, graphY = 0, graphW = 0, graphH = 0;
    float graphMinTemp = 0;
    float graphMaxTemp = 0;
    uint32_t graphTotalTime = 0;
};

extern SolderProfile solderProfile;

