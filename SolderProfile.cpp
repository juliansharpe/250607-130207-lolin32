#include "SolderProfile.h"

// Example profile values, adjust as needed
#define PHASE_MS(x) ((x) * 1000)

SolderProfile::SolderProfile()
    : phases{
        Phase( 25, 150, PHASE_MS(120), PHASE_MS(120)),   // Preheat
        Phase(150, 180, PHASE_MS( 60), PHASE_MS(120)),   // Soak
        Phase(180, 220, PHASE_MS( 30), PHASE_MS( 60)),   // Peak
        Phase(220, 200, PHASE_MS( 20), PHASE_MS( 20))    // Dwell (endTemp can be lower for cooldown)
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

SolderProfile solderProfile;
