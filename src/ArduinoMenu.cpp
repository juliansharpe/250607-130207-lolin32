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

  // --- Prime: Heat chamber to +5C above current temp, max 30s ---
  digitalWrite(fan,1); // Turn off the fan
  digitalWrite(mainElement,1);
  digitalWrite(fryerElement,1);

  float primeStartTemp = GetFilteredTemp(ReadTemp(true));
  float primeTarget = primeStartTemp + 5.0f;
  unsigned long primeStart = millis();
  const unsigned long primeTimeout = 30000; // 30 seconds

  while (millis() - primeStart < primeTimeout) {
    float t = GetFilteredTemp(ReadTemp(false));
    gfx.setTextColor(TFT_NAVY, TFT_BLACK);
    gfx.setTextSize(1);
    gfx.setCursor(0, 0);
    gfx.printf("Priming: %.1f/%.1fC   ", t, primeTarget);
    if (t >= primeTarget) break;
    delay(100);
  }

  digitalWrite(mainElement,0);
  digitalWrite(fryerElement,0);

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
  float feedForwardAccumulator = -1.0;

  // --- Track error statistics ---
  float diffSum = 0.0f;
  float diffMax = 0.0f;
  uint32_t diffCount = 0;

  while( solderProfile.currentPhase() != SolderProfile::COMPLETE) {    
    unsigned long elapsed = millis() - readtime;
    if( elapsed > 250) {
      readtime = millis();
      temp = GetFilteredTemp(ReadTemp(false));

      // Update the PID target temperature based on the current phase
      float setpoint = solderProfile.getSetpoint();
      SetPIDTargetTemp(setpoint);

      // Get PID output and control elements
      float pidOutput = GetPIDOutput(temp);

      // Add feed-forward control based on the current phase
      const float maxHeatRate = 100.0 / 120.0; // 100 degrees in 120 seconds

      uint32_t nowMs = millis();
      float feedForwardSlope = solderProfile.getFeedForwardSlope(15000); // seconds for feed-forward slope
      float feedForwardPower = (feedForwardSlope / maxHeatRate) * 100.0; // Scale to 0-100
      feedForwardPower += setpoint / 10; // add term proportional to temperature

      feedForwardAccumulator = feedForwardPower; 

      // Adjust PID output with feed-forward control
      pidOutput += feedForwardAccumulator;
      pidOutput = constrain(pidOutput, 0, 100); // Ensure output is within bounds

      // --- Track error statistics ---
      float diff = temp - setpoint;
      diffSum += fabs(diff);
      if (fabs(diff) > diffMax) diffMax = fabs(diff);
      diffCount++;

      // Update the solder profile with the current temperature
      solderProfile.update(temp, pidOutput); 

      Serial.printf(
        "P:%s Temp:(A:%.1f,S:%.1f,D:%.1f) Temp Stats(A:%.1f M:%.1f) FF:%.1f Out:%.0f (P:%.0f,I:%.0f,D:%.0f,F:%.0f)\n",
        solderProfile.phases[solderProfile.currentPhase()].phaseName,
        temp, setpoint, diff,
        (diffCount > 0 ? diffSum / diffCount : 0.0f), diffMax,
        feedForwardSlope,
        pidOutput,
        myPID.GetLastP(), myPID.GetLastI(), myPID.GetLastD(), feedForwardPower
      );

      gfx.setTextColor(TFT_NAVY, TFT_BLACK);
      gfx.setTextSize(1);
      gfx.setCursor(0, 0);
      gfx.printf("%.0fC %.0f:(%.0f,%.0f,%.0f,%.0f)    ", temp, pidOutput, myPID.GetLastP(), myPID.GetLastI(), myPID.GetLastD(), feedForwardPower);

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

