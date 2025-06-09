#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AiEsp32RotaryEncoder.h>
#include "MenuConfig.h"
#include <Temp.h>
#include "ArduinoMenu.h"
#include "SolderProfile.h"

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(21,22, 5, -1, 2);

TFT_eSPI gfx;
const int fan =           25;
const int fryerElement =  26;
const int mainElement =   27;

void IRAM_ATTR readEncoderISR()
{
	rotaryEncoder.readEncoder_ISR();
}

void setup() {
  Serial.begin(115200);

  // Setup SSR pins
  pinMode(mainElement, OUTPUT);
  pinMode(fryerElement, OUTPUT);
  pinMode(fan, OUTPUT);
  digitalWrite(mainElement,0);
  digitalWrite(fryerElement,0);
  digitalWrite(fan,1);

  // Initialize the screen
  gfx.init();
  gfx.setRotation(3);
  gfx.setTextWrap(false);
  gfx.fillScreen(Black);
  gfx.setTextColor(Red,Black);
  gfx.setTextSize(1);

  // Initialize the rotary encoder
	SPI.begin();
  rotaryEncoder.begin();
	rotaryEncoder.setup(readEncoderISR);
	rotaryEncoder.setBoundaries(-1000000, 1000000, true);
	rotaryEncoder.setAcceleration(50);
}

void StartReflowProfile(ReflowProfile& profile) {
  // Start the reflow profile
  Serial.printf("Starting reflow profile: Preheat %dC, Soak %dC, Peak %dC, Dwell %ds\n",
                profile.preheatTemp, profile.soakTemp, profile.peakTemp, profile.dwellTime);
  
  gfx.fillScreen(Black);
  gfx.setTextColor(Blue,Black);
  gfx.setTextSize(1);
 
  // Prime
  digitalWrite(fan,1); // Turn off the fan
  digitalWrite(mainElement,1);
  digitalWrite(fryerElement,1);
  delay(20000);


  float temp=0;
  uint32_t startPWMTime = 0;
  uint16_t PWMMain = 0;
  uint16_t PWMFryer = 0;
  const uint32_t MaxPWM = 99;
  InitPID();  

  solderProfile.startReflow();
  temp = GetFilteredTemp(ReadTemp(true));  
  solderProfile.phases[0].startTemp = temp;
  solderProfile.initGraph(gfx, 0, 14, GFX_WIDTH, GFX_HEIGHT-14);
  solderProfile.drawGraph();

  uint32_t delta=0; 

  digitalWrite(fan,1); // Turn on the fan
  unsigned long readtime = millis();

  while( solderProfile.currentPhase() != SolderProfile::COMPLETE) {    
    unsigned long elapsed = millis() - readtime;
    if( elapsed > 250) {
      readtime = millis();
      temp = GetFilteredTemp(ReadTemp(false));
      gfx.setTextColor(TFT_NAVY, TFT_BLACK);
      gfx.setTextSize(1);
      gfx.setCursor(0, 0);
      gfx.printf("%.1fC           ", temp);

      // Update the PID target temperature based on the current phase
      float setpoint = solderProfile.getSetpoint();
      SetPIDTargetTemp(setpoint);
    
      // Update the solder profile with the current temperature
      solderProfile.update(temp);
    
      // Redraw the graph
      //solderProfile.drawGraph();

      // Get PID output and control elements
      float pidOutput = GetPIDOutput(temp);

      Serial.printf("Phase: %s, Temp: %.1fC, Setpoint: %.1fC, PID Output: %.1f%%\n",
        solderProfile.phases[solderProfile.currentPhase()].phaseName,
        temp, setpoint, pidOutput);

      if( pidOutput > 50) {
        PWMMain = 100;
        PWMFryer = constrain((pidOutput-50) * 2, 0, 100);
      } else {
        PWMMain = constrain(pidOutput * 2, 0, 100);
        PWMFryer = 0;
      }
    }
     
    uint32_t delta = (millis() - startPWMTime) / 10;
    if(delta > MaxPWM) {
      startPWMTime = millis();
      delta = 0;
      digitalWrite(mainElement,1);
      digitalWrite(fryerElement,1);
    }

    if( delta >= PWMMain ) {
      digitalWrite(mainElement,0);
    }

    if( delta >= PWMFryer ) {
      digitalWrite(fryerElement,0);
    }
  }
}

void loop() {
  menuLoop(rotaryEncoder);
  gfx.setTextColor(TFT_NAVY, TFT_BLACK);
  gfx.setTextSize(1);
  gfx.setCursor(0, 0);
  gfx.printf("%.0fC           ", GetFilteredTemp(ReadTemp(true)));
}

