#include <Arduino.h>
#include <TFT_eSPI.h>
#include <menu.h>
#include <menuIO/serialIO.h>
#include <menuIO/TFT_eSPIOut.h>
#include <menuIO/chainStream.h>
#include <menuIO/rotaryEventIn.h>
#include <AiEsp32RotaryEncoder.h>

// Function prototypes
void setup();
void loop();
void IRAM_ATTR readEncoderISR();


AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(21,22, 5, -1, 2);

// === Display ===
TFT_eSPI gfx;

#define GFX_WIDTH 160
#define GFX_HEIGHT 128
#define MAX_DEPTH 3
#define fontW 7
#define fontH 20

RotaryEventIn reIn(
  RotaryEventIn::EventType::BUTTON_CLICKED | // select
  RotaryEventIn::EventType::BUTTON_DOUBLE_CLICKED | // back
  RotaryEventIn::EventType::BUTTON_LONG_PRESSED | // also back
  RotaryEventIn::EventType::ROTARY_CCW | // up
  RotaryEventIn::EventType::ROTARY_CW // down
); // register capabilities, see AndroidMenu MenuIO/RotaryEventIn.h file
MENU_INPUTS(in,&reIn);

// === Reflow Profile Data ===
struct ReflowProfile {
  int preheatTemp;
  int soakTemp;
  int peakTemp;
  int dwellTime;
};

ReflowProfile profiles[] = {
  {150, 180, 220, 60}, // Lead-Free
  {130, 160, 190, 50},  // Leaded
  {130, 160, 190, 50},  // Custom 1
  {130, 160, 190, 50}   // Custom 2
};

const char* profileNames[] = { "Lead-Free", "Leaded" };

// === Settings Variables ===
int ovenTemp = 80;
int Time = 60;

// === Menu Setup ===
using namespace Menu;

// === Submenus for each profile ===
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

// === Dynamic Profile Selector ===
MENU(profileMenu, "Edit Profile",doNothing,noEvent,wrapStyle
  ,SUBMENU(leadFreeMenu)
  ,SUBMENU(leadedMenu)
  ,SUBMENU(custom1Menu)
  ,SUBMENU(custom2Menu)
  ,EXIT("< Back")
);

MENU(reflowStartMenu, "Start Reflow",doNothing,noEvent,wrapStyle
  ,OP("Lead-Free >",doNothing,noEvent)
  ,OP("Leaded >",doNothing,noEvent)
  ,OP("Custom 1 >",doNothing,noEvent)
  ,OP("Custom 2 >",doNothing,noEvent)
  ,EXIT("< Back")
);

MENU(ovenStartMenu, "Start Oven",doNothing,noEvent,wrapStyle
  ,FIELD(ovenTemp, "Temp", "C", 0, 250, 1, 0, doNothing,noEvent,noStyle)
  ,FIELD(Time, "Max time", "Mins", 0, 1440, 1, 0, doNothing,noEvent,noStyle)
  ,OP("Cook >",doNothing,noEvent)
  ,EXIT("< Back")
);

MENU(mainMenu, "Workshop Oven",doNothing,noEvent,wrapStyle
  ,SUBMENU(reflowStartMenu) // Start Reflow
  ,SUBMENU(ovenStartMenu)   // Start Oven
  ,SUBMENU(profileMenu)     // Edit Profile
  ,EXIT("<Exit")
);

#define Black RGB565(0,0,0)
#define Red	RGB565(255,0,0)
#define Green RGB565(0,255,0)
#define Blue RGB565(0,0,255)
#define Gray RGB565(128,128,128)
#define LighterRed RGB565(255,150,150)
#define LighterGreen RGB565(150,255,150)
#define LighterBlue RGB565(150,150,255)
#define DarkerRed RGB565(150,0,0)
#define DarkerGreen RGB565(0,150,0)
#define DarkerBlue RGB565(0,0,150)
#define Cyan RGB565(0,255,255)
#define Magenta RGB565(255,0,255)
#define Yellow RGB565(255,255,0)
#define White RGB565(255,255,255)

const colorDef<uint16_t> colors[6] MEMMODE={
  {{(uint16_t)Black,(uint16_t)Black}, {(uint16_t)Black, (uint16_t)Black,  (uint16_t)Black}},//bgColor
  {{(uint16_t)Gray, (uint16_t)Gray},  {(uint16_t)Gray, (uint16_t)White, (uint16_t)White}},//fgColor
  {{(uint16_t)White,(uint16_t)Black}, {(uint16_t)Gray,(uint16_t)White,(uint16_t)Blue}},//valColor
  {{(uint16_t)White,(uint16_t)Black}, {(uint16_t)Gray, (uint16_t)White,(uint16_t)Blue}},//unitColor
  {{(uint16_t)White,(uint16_t)Gray},  {(uint16_t)Black, (uint16_t)Black,  (uint16_t)White}},//cursorColor
  {{(uint16_t)White,(uint16_t)Yellow},{(uint16_t)Black,  (uint16_t)Red,   (uint16_t)Red}},//titleColor
};

//define serial output device
idx_t serialTops[MAX_DEPTH]={0};
serialOut outSerial(Serial,serialTops);

const panel panels[] MEMMODE = {{0, 0, GFX_WIDTH / fontW, GFX_HEIGHT / fontH}};
navNode* nodes[sizeof(panels) / sizeof(panel)]; //navNodes to store navigation status
panelsList pList(panels, nodes, 1); //a list of panels and nodes
idx_t eSpiTops[MAX_DEPTH]={0};
TFT_eSPIOut eSpiOut(gfx,colors,eSpiTops,pList,fontW,fontH+1);
menuOut* constMEM outputs[] MEMMODE={&outSerial,&eSpiOut};//list of output devices
outputsList out(outputs,sizeof(outputs)/sizeof(menuOut*));//outputs list controller

NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);

void IRAM_ATTR readEncoderISR()
{
	rotaryEncoder.readEncoder_ISR();
}

void setup() {
  Serial.begin(115200);

  SPI.begin();
  gfx.init();
  gfx.setRotation(3);
  gfx.setTextWrap(false);
  gfx.fillScreen(Black);
  gfx.setTextColor(Red,Black);
  gfx.setTextSize(1);
  // gfx.setFreeFont(FSS9);

	rotaryEncoder.begin();
	rotaryEncoder.setup(readEncoderISR);
	rotaryEncoder.setBoundaries(-1000000, 1000000, true); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
	rotaryEncoder.setAcceleration(50); //or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration

  nav.idleOn();
}

void loop() {
  static long last, val;
  if (rotaryEncoder.encoderChanged()) {
    val = rotaryEncoder.readEncoder();
    nav.selected();
    nav.doNav(val > last ? downCmd : upCmd);
    last = val;
  }

  if (rotaryEncoder.isEncoderButtonClicked()) {
    nav.doNav(enterCmd);
  }

  nav.poll();
  delay(10);
}

