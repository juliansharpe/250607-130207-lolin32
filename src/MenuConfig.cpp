#include "MenuConfig.h"
#include "Free_Fonts.h"
#include "ArduinoMenu.h"

// === Reflow Profile Data ===
ReflowProfile profiles[] = {
  {150, 180, 220, 60}, // Lead-Free
  {130, 160, 190, 50},  // Leaded
  {130, 160, 190, 50},  // Custom 1
  {130, 160, 190, 50}   // Custom 2
};

const char* profileNames[] = { "Lead-Free", "Leaded", "Custom 1", "Custom 2" };

// === Settings Variables ===
int ovenTemp = 80;
int Time = 60;

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
  ,FIELD(profiles[0].preheatTemp, "Preheat", "C", 100, 200, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[0].soakTemp, "Soak", "C", 120, 220, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[0].peakTemp, "Peak", "C", 180, 250, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[0].dwellTime, "Dwell", "s", 20, 120, 1, 0, doNothing,noEvent,noStyle)
  ,EXIT("< Back")
);

MENU(leadedMenu, "Leaded",doNothing,noEvent,wrapStyle
  ,FIELD(profiles[1].preheatTemp, "Preheat", "C", 100, 180, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[1].soakTemp, "Soak", "C", 120, 200, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[1].peakTemp, "Peak", "C", 160, 230, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[1].dwellTime, "Dwell", "s", 20, 120, 1, 0, doNothing,noEvent,noStyle)
  ,EXIT("< Back")
);

MENU(custom1Menu, "Custom 1",doNothing,noEvent,wrapStyle
  ,FIELD(profiles[2].preheatTemp, "Preheat", "C", 100, 180, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[2].soakTemp, "Soak", "C", 120, 200, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[2].peakTemp, "Peak", "C", 160, 230, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[2].dwellTime, "Dwell", "s", 20, 120, 1, 0, doNothing,noEvent,noStyle)
  ,EXIT("< Back")
);

MENU(custom2Menu, "Custom 2",doNothing,noEvent,wrapStyle
  ,FIELD(profiles[3].preheatTemp, "Preheat", "C", 100, 180, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[3].soakTemp, "Soak", "C", 120, 200, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[3].peakTemp, "Peak", "C", 160, 230, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(profiles[3].dwellTime, "Dwell", "s", 20, 120, 1, 0, doNothing,noEvent,noStyle)
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
  ,OP("Start Lead-Free",onProfileStart,enterEvent)
  ,OP("Start Leaded",onProfileStart,enterEvent)
  ,OP("Start Custom 1",onProfileStart,enterEvent)
  ,OP("Start Custom 2",onProfileStart,enterEvent)
  ,EXIT("< Back")
);

MENU(ovenStartMenu, "Start Oven",doNothing,noEvent,wrapStyle
  ,FIELD(ovenTemp, "Temp", "C", 0, 250, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(Time, "Max time", "Mins", 0, 1440, 1, 0, doNothing,noEvent,noStyle)
  ,OP("Cook >",doNothing,noEvent)
  ,EXIT("< Back")
);

MENU(mainMenu, "Workshop Oven",doNothing,noEvent,wrapStyle
  ,SUBMENU(reflowStartMenu)
  ,SUBMENU(ovenStartMenu)
  ,SUBMENU(profileMenu)
  ,EXIT("<Exit")
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

// === New function for profile start ===
result onProfileStart(eventMask e, navNode& nav, prompt &item) {
  Serial.print("Starting profile: ");
  Serial.println(item.getText());
  // Add any additional logic here
  StartReflowProfile( profiles[0] );
  return proceed;
}
 