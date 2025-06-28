#pragma once

#include <stdint.h>
#include <TFT_eSPI.h>

#define SOLDER_PROFILE_MAX_PHASES 5

struct SolderProfileParams {
    struct PhaseParam {
        const char* phaseName;
        float startTemp;
        float endTemp;
        uint32_t minTimeMs;
        uint32_t maxTimeMs;
        bool maxRate;
    };
    PhaseParam phases[SOLDER_PROFILE_MAX_PHASES];
    uint8_t numPhases;
};

class SolderProfile {
public:
    enum PhaseType { PREHEAT, SOAK, PEAK, DWELL, COOL, COMPLETE };

    struct Phase {
        const char* phaseName;
        float startTemp;
        float endTemp;
        float achievedTemp;
        uint32_t minTimeMs;
        uint32_t maxTimeMs;
        uint32_t startTimeMs;
        bool maxRate;
        bool completed;

        Phase(const char* name, float s, float e, float minT, float maxT, bool maxR = false)
            : phaseName(name), startTemp(s), endTemp(e), achievedTemp(e*0.98), minTimeMs(minT), maxTimeMs(maxT), startTimeMs(0), maxRate(maxR), completed(false) {}
        Phase() : phaseName(""), startTemp(0), endTemp(0), achievedTemp(0), minTimeMs(0), maxTimeMs(0), startTimeMs(0), maxRate(false), completed(false) {}
    };

    SolderProfile();
    SolderProfile(const SolderProfileParams& params);
    void setProfile(const SolderProfileParams& params);

    void startReflow();
    void update(float actualTemp, float output);
    PhaseType currentPhase() const;
    bool isComplete() const;
    float getIdealTemp();
    float getSetpoint();
    float getFeedForwardSlope(uint32_t deltaMs);

    // Must call initGraph before drawGraph
    void initGraph(TFT_eSPI& tft, int x, int y, int w, int h);
    void drawGraph();

    Phase phases[SOLDER_PROFILE_MAX_PHASES];
    uint8_t numPhases;
private:
    PhaseType phaseIdx;
    void nextPhase(uint32_t nowMs);

    // --- Graph state ---
    TFT_eSPI* tftRef = nullptr;
    int graphX = 0, graphY = 0, graphW = 0, graphH = 0;
    float graphMinTemp = 0;
    float graphMaxTemp = 0;
    uint32_t graphTotalTime = 0;

    // --- Reflow timing ---
    uint32_t reflowStartTime = 0;
};

extern SolderProfile solderProfile;

