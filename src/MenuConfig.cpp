#include "MenuConfig.h"
#include "Free_Fonts.h"
#include "ArduinoMenu.h"
#include <Preferences.h>

// === Reflow Profile Data ===
ReflowProfile profiles[] = {
  {150, 180, 220, 60}, // Lead-Free
  {130, 160, 190, 50},  // Leaded
  {130, 160, 190, 50},  // Low temp
  {130, 160, 190, 50}   // Custom 2
};

const char* profileNames[] = { "Lead-Free", "Leaded", "Low temp", "Custom 2" };

// === Settings Variables ===
int ovenTemp = 0;
int Time = 15;

// Define rotary input
RotaryEventIn reIn(
  RotaryEventIn::EventType::BUTTON_CLICKED |
  RotaryEventIn::EventType::BUTTON_DOUBLE_CLICKED |
  RotaryEventIn::EventType::BUTTON_LONG_PRESSED |
  RotaryEventIn::EventType::ROTARY_CCW |
  RotaryEventIn::EventType::ROTARY_CW
);
MENU_INPUTS(in,&reIn);

MENU(leadFreeMenu, "Lead-Free",doNothing,noEvent,wrapStyle
  ,FIELD(profiles[0].preheatTemp, "Preheat", "C", 100, 200, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[0].soakTemp, "Soak", "C", 120, 220, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[0].peakTemp, "Peak", "C", 180, 250, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[0].dwellTime, "Dwell", "s", 20, 120, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,EXIT("< Back")
);

MENU(leadedMenu, "Leaded",doNothing,noEvent,wrapStyle
  ,FIELD(profiles[1].preheatTemp, "Preheat", "C", 100, 180, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[1].soakTemp, "Soak", "C", 120, 200, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[1].peakTemp, "Peak", "C", 160, 230, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[1].dwellTime, "Dwell", "s", 20, 120, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,EXIT("< Back")
);

MENU(custom1Menu, "183 Low Temp",doNothing,noEvent,wrapStyle
  ,FIELD(profiles[2].preheatTemp, "Preheat", "C", 100, 180, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[2].soakTemp, "Soak", "C", 120, 200, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[2].peakTemp, "Peak", "C", 160, 230, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[2].dwellTime, "Dwell", "s", 20, 120, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,EXIT("< Back")
);

MENU(custom2Menu, "Custom 2",doNothing,noEvent,wrapStyle
  ,FIELD(profiles[3].preheatTemp, "Preheat", "C", 100, 180, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[3].soakTemp, "Soak", "C", 120, 200, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[3].peakTemp, "Peak", "C", 160, 230, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,FIELD(profiles[3].dwellTime, "Dwell", "s", 20, 120, 1, 0, saveProfilesToFlash,exitEvent,noStyle)
  ,EXIT("< Back")
);

MENU(profileMenu, "Edit Profile",doNothing,noEvent,wrapStyle
  ,SUBMENU(leadFreeMenu)
  ,SUBMENU(leadedMenu)
  ,SUBMENU(custom1Menu)
  ,SUBMENU(custom2Menu)
  ,EXIT("< Back")
);

MENU(reflowStartMenu, "Start Reflow",doNothing,noEvent,wrapStyle
  ,OP("Start Lead-Free",onStartLeadFree,enterEvent)
  ,OP("Start Leaded",onStartLeaded,enterEvent)
  ,OP("Start Low temp",onStartLowTemp,enterEvent)
  ,OP("Start Custom 2",onStartCustom2,enterEvent)
  ,EXIT("< Back")
);

MENU(mainMenu, "Workshop Oven",doNothing,noEvent,wrapStyle
  ,SUBMENU(reflowStartMenu)
  ,SUBMENU(profileMenu)
  ,OP("Start Oven >",onStartOven,enterEvent)
);

idx_t serialTops[MAX_DEPTH]={0};
serialOut outSerial(Serial,serialTops);

const panel panels[] MEMMODE = {{0, 0, GFX_WIDTH / fontW, GFX_HEIGHT / fontH}};
navNode* nodes[sizeof(panels) / sizeof(panel)];
panelsList pList(panels, nodes, 1);
idx_t eSpiTops[MAX_DEPTH]={0};
TFT_eSPIOut eSpiOut(gfx,colors,eSpiTops,pList,fontW,fontH+1);
menuOut* constMEM outputs[] MEMMODE={&outSerial,&eSpiOut};
outputsList out(outputs,sizeof(outputs)/sizeof(menuOut*));

NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);

void menuLoop(AiEsp32RotaryEncoder& rotaryEncoder) {
  static long last, val;
  if (rotaryEncoder.encoderChanged()) {
    val = rotaryEncoder.readEncoder();
    nav.doNav(val > last ? downCmd : upCmd);
    last = val;
  }

  if (rotaryEncoder.isEncoderButtonClicked()) {
    nav.doNav(enterCmd);
  }

  nav.poll();
  delay(10);
}

// === Functions for each Start action ===
result onStartLeadFree(eventMask e, navNode& nav, prompt &item) {
  StartReflowProfile(profiles[0]);
  gfx.fillScreen(Black);
  gfx.setTextSize(2);
  return quit;
}
result onStartLeaded(eventMask e, navNode& nav, prompt &item) {
  StartReflowProfile(profiles[1]);
  gfx.fillScreen(Black);
  gfx.setTextSize(2);
  return quit;
}
result onStartLowTemp(eventMask e, navNode& nav, prompt &item) {
  StartReflowProfile(profiles[2]);
  gfx.fillScreen(Black);
  gfx.setTextSize(2);
  return quit;
}
result onStartCustom2(eventMask e, navNode& nav, prompt &item) {
  StartReflowProfile(profiles[3]);
  gfx.fillScreen(Black);
  gfx.setTextSize(2);
  return quit;
}
result onStartOven(eventMask e, navNode& nav, prompt &item) {
  StartOven();
  gfx.fillScreen(Black);
  gfx.setTextSize(2);
  return quit;
}

Preferences preferences;

// Helper: Save all profiles to flash
void saveProfilesToFlash() {
  Serial.println("Saving profiles to flash...");
  preferences.begin("reflow", false);
  for (int i = 0; i < 4; ++i) {
    char key[16];
    snprintf(key, sizeof(key), "p%d_preheat", i);
    preferences.putInt(key, profiles[i].preheatTemp);
    snprintf(key, sizeof(key), "p%d_soak", i);
    preferences.putInt(key, profiles[i].soakTemp);
    snprintf(key, sizeof(key), "p%d_peak", i);
    preferences.putInt(key, profiles[i].peakTemp);
    snprintf(key, sizeof(key), "p%d_dwell", i);
    preferences.putInt(key, profiles[i].dwellTime);
  }
  preferences.end();
}

// Helper: Load all profiles from flash (if present)
void loadProfilesFromFlash() {
  Serial.println("Loading profiles to flash...");
  preferences.begin("reflow", true);
  for (int i = 0; i < 4; ++i) {
    char key[16];
    snprintf(key, sizeof(key), "p%d_preheat", i);
    int preheat = preferences.getInt(key, profiles[i].preheatTemp);
    snprintf(key, sizeof(key), "p%d_soak", i);
    int soak = preferences.getInt(key, profiles[i].soakTemp);
    snprintf(key, sizeof(key), "p%d_peak", i);
    int peak = preferences.getInt(key, profiles[i].peakTemp);
    snprintf(key, sizeof(key), "p%d_dwell", i);
    int dwell = preferences.getInt(key, profiles[i].dwellTime);
    profiles[i].preheatTemp = preheat;
    profiles[i].soakTemp = soak;
    profiles[i].peakTemp = peak;
    profiles[i].dwellTime = dwell;
  }
  preferences.end();
}

