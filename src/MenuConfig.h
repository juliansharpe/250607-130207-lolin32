#pragma once

#include <menu.h>
#include <menuIO/serialIO.h>
#include <myTFT_eSPIOut.h>
#include <menuIO/chainStream.h>
#include <menuIO/rotaryEventIn.h>
#include <ColorDef.h>
#include <TFT_eSPI.h>
#include <AiEsp32RotaryEncoder.h>

// === Display ===
#define GFX_WIDTH 160
#define GFX_HEIGHT 128
#define MAX_DEPTH 3
#define fontW 8
#define fontH 20

// === Reflow Profile Data ===
struct ReflowProfile {
  int preheatTemp;
  int soakTemp;
  int peakTemp;
  int dwellTime;
};

extern ReflowProfile profiles[];
extern const char* profileNames[];

// === Settings Variables ===
extern int ovenTemp;
extern int Time;

// === Menu Setup ===
using namespace Menu;

extern TFT_eSPI gfx;
extern RotaryEventIn reIn;
extern Menu::navRoot nav;
extern Menu::menu mainMenu;

//extern NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);

void menuLoop(AiEsp32RotaryEncoder& rotaryEncoder);

// New per-profile start handlers
result onStartLeadFree(eventMask e, navNode& nav, prompt &item);
result onStartLeaded(eventMask e, navNode& nav, prompt &item);
result onStartLowTemp(eventMask e, navNode& nav, prompt &item);
result onStartCustom2(eventMask e, navNode& nav, prompt &item);
result onStartOven(eventMask e, navNode& nav, prompt &item);
void saveProfilesToFlash();
void loadProfilesFromFlash();

// Optionally remove or comment out this if not used anymore
// result onProfileStart(eventMask e, navNode& nav, prompt &item);
