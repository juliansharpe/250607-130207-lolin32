#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AiEsp32RotaryEncoder.h>
#include "MenuConfig.h"
#include <Temp.h>
#include "ArduinoMenu.h"
#include "SolderProfile.h"

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(21,22, 5, -1, 2);

TFT_eSPI gfx;

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

	rotaryEncoder.begin();
	rotaryEncoder.setup(readEncoderISR);
	rotaryEncoder.setBoundaries(-1000000, 1000000, true);
	rotaryEncoder.setAcceleration(50);
}

void StartReflowProfile(ReflowProfile& profile) {
  // Start the reflow profile
  Serial.printf("Starting reflow profile: Preheat %dC, Soak %dC, Peak %dC, Dwell %ds\n",
                profile.preheatTemp, profile.soakTemp, profile.peakTemp, profile.dwellTime);
  
  solderProfile.startReflow();
  gfx.fillScreen(Black);
  gfx.setTextColor(Blue,Black);
  gfx.setTextSize(1);
 
  solderProfile.initGraph(gfx, 0, 14, GFX_WIDTH, GFX_HEIGHT-14);
  solderProfile.drawGraph();

  TempInit();

  unsigned long startTime = millis();
  while(1==1) {
    delay(250);
    unsigned long elapsedms = (millis() - startTime) * 10;
    float temp = solderProfile.getIdealTemp() * 0.9;
    
    SolderProfile::Phase currentPhase = solderProfile.phases[solderProfile.currentPhase()];
    gfx.setTextSize(1);
    gfx.setCursor(0, 0);
    gfx.printf("%s: (%.0f - %.0fC) ", currentPhase.phaseName, 
               currentPhase.startTemp, currentPhase.endTemp);
    gfx.printf(" %.0fC", temp);
    solderProfile.update(temp);
  }
}

void loop() {
  menuLoop(rotaryEncoder);
}

