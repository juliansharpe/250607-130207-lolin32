#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AiEsp32RotaryEncoder.h>
#include "MenuConfig.h"
#include <Temp.h>
#include "ArduinoMenu.h"
#include "SolderProfile.h"
#include "Free_Fonts.h"
#include "Oven.h"
#include "ElementPWM.h"

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
  gfx.setTextFont(2);

  // Initialize the rotary encoder
	SPI.begin();
  rotaryEncoder.begin();
	rotaryEncoder.setup(readEncoderISR);
	rotaryEncoder.setBoundaries(-1000000, 1000000, true);
	rotaryEncoder.setAcceleration(50);

  InitTempSensor();
}

void StartReflowProfile(ReflowProfile& profile) {
  // Convert ReflowProfile to SolderProfileParams (simple 4-phase profile)
  SolderProfileParams params;
  params.phases[0] = {"Preheat", 0, (float)profile.preheatTemp, 180000, 180000, false};
  params.phases[1] = {"Soak", (float)profile.preheatTemp, (float)profile.soakTemp, 120000, 120000, false};
  params.phases[2] = {"Peak", (float)profile.soakTemp, (float)profile.peakTemp, 70000, 120000, true};
  params.phases[3] = {"Dwell", (float)profile.peakTemp, (float)profile.peakTemp, (uint32_t)profile.dwellTime*1000, (uint32_t)profile.dwellTime*1000, false};
  params.phases[4] = {"Cool", (float)profile.peakTemp, 0, 90000, 90000, true};
  params.numPhases = 5;

  solderProfile.setProfile(params);

  Serial.printf("Starting reflow profile: Preheat %dC, Soak %dC, Peak %dC, Dwell %ds\n",
                profile.preheatTemp, profile.soakTemp, profile.peakTemp, profile.dwellTime);
  
  gfx.fillScreen(Black);
  gfx.setTextColor(Blue,Black);
  gfx.setTextSize(1);

  float temp=0;
  InitPID();  

  solderProfile.startReflow();
  temp = GetFilteredTemp(ReadTemp(true));  
  solderProfile.phases[0].startTemp = temp;
  solderProfile.initGraph(gfx, 0, 14, GFX_WIDTH, GFX_HEIGHT-14);
  solderProfile.drawGraph();

  digitalWrite(fan,1); // Turn on the fan
  unsigned long readtime = millis();
  float feedForwardAccumulator = -1000.0;

  // --- Track error statistics ---
  float diffSum = 0.0f;
  float diffMax = 0.0f;
  uint32_t diffCount = 0;

  // --- Use ElementPWM for SSR control ---
  ElementPWM elementPWM(mainElement, fryerElement, 1000); // 1Hz PWM
  uint8_t PWMMain = 0;
  uint8_t PWMFryer = 0;

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
      float feedForwardSlope = solderProfile.getFeedForwardSlope(20000); // seconds for feed-forward slope
      float feedForwardPower = (feedForwardSlope / maxHeatRate) * 100.0; // Scale to 0-100
      feedForwardPower += setpoint / 10; // add term proportional to temperature

      if( feedForwardAccumulator < -999.0) {
        feedForwardAccumulator = feedForwardPower;
      } else {
        // Smooth the feed-forward control
        feedForwardAccumulator = (0.0f * feedForwardAccumulator) + (1.0f * feedForwardPower);
      }

      // Adjust PID output with feed-forward control
      pidOutput += feedForwardAccumulator;
      pidOutput = constrain(pidOutput, 0, 100); // Ensure output is within bounds

      // --- Track error statistics ---
      float diff = temp - setpoint;
      diffSum += fabs(diff);
      diffMax = diffMax * 0.999 + (fabs(diff)*0.001);
      diffCount++;

      // Update the solder profile with the current temperature
      solderProfile.update(temp, pidOutput); 

      Serial.printf(
        "P:%s Temp:(A:%.1f,S:%.1f,D:%.1f) TStats(A:%.1f M:%.1f) FF:%.1f Out:%.0f (P:%.0f,I:%.0f,D:%.0f,F:%.0f)\n",
        solderProfile.phases[solderProfile.currentPhase()].phaseName,
        temp, setpoint, diff,
        (diffCount > 0 ? diffSum / diffCount : 0.0f), diffMax,
        feedForwardAccumulator,
        pidOutput,
        myPID.GetLastP(), myPID.GetLastI(), myPID.GetLastD(), feedForwardPower
      );

      gfx.setTextColor(TFT_BLUE, TFT_BLACK);
      gfx.setTextSize(1);
      gfx.setCursor(0, 0);
      gfx.printf("%.0fC %.0f:(%.0f,%.0f,%.0f,%.0f)    ", temp, pidOutput, myPID.GetLastP(), myPID.GetLastI(), myPID.GetLastD(), feedForwardAccumulator);

      if( pidOutput > 50) {
        PWMMain = 100;
        PWMFryer = constrain((pidOutput-50) * 2, 0, 100);
      } else {
        PWMMain = constrain(pidOutput * 2, 0, 100);
        PWMFryer = 0;
      }
      // Set PWM levels for this cycle
      elementPWM.setPWM(PWMMain, PWMFryer);
    }
    // Regularly update the PWM outputs
    elementPWM.process();
  }
}

void WriteTemp(float temp, float settemp, bool isEditing)
{
    gfx.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    
    gfx.printf("Oven:%.0fc/", temp);
    if (isEditing) {
            gfx.setTextColor(TFT_BLUE, TFT_BLACK);

    } else {
            gfx.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    }
    gfx.printf("%.0fc ", settemp);   
}

void WriteTime(int setTimeMins, bool isEditing)
{
    gfx.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    gfx.printf("Timer:");

    if (isEditing) {
        gfx.setTextColor(TFT_BLUE, TFT_BLACK);
    } else {
        gfx.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    }
    if (setTimeMins == 0) {
        gfx.printf("Off");
    } else {
        int hrs = setTimeMins / 60;
        int mins = setTimeMins % 60;
        if (hrs > 0) {
            gfx.printf("%d:%02d    ", hrs, mins);
        } else {
            gfx.printf("%dm    ", mins);
        }
    }
}

void StartOven(float defaultTemp, uint32_t maxTimeMs) {
    Oven oven(defaultTemp, maxTimeMs);
    gfx.fillScreen(Black);
    gfx.setTextColor(Blue, Black);
    gfx.setTextFont(1);
    gfx.setTextSize(1);
    oven.initGraph(gfx, 0, 14, GFX_WIDTH-1, GFX_HEIGHT-14);

    float temp = 0;
    float setTemp = defaultTemp;
    int setTimeMins = 60; // default 60 minutes
    unsigned long readtime = millis();
    oven.reset();
    enum EditMode { NONE, TEMP, TIME };
    EditMode editMode = NONE;
    long lastEncoder = rotaryEncoder.readEncoder();

    while (true) {
        unsigned long elapsed = millis() - readtime;
        if (elapsed > 250) {
            readtime = millis();
            temp = GetFilteredTemp(ReadTemp(false));
            oven.updateGraph(temp);
        }

        gfx.setTextSize(1);
        gfx.setCursor(0, 0);
        WriteTemp(temp, setTemp, editMode == TEMP);
        WriteTime(setTimeMins, editMode == TIME);

        // Handle rotary button click to cycle edit modes
        if (rotaryEncoder.isEncoderButtonClicked()) {
            if (editMode == NONE) {
                editMode = TEMP;
            } else if (editMode == TEMP) {
                editMode = TIME;
            } else if (editMode == TIME) {
                editMode = NONE;
                if (setTimeMins > 0) {
                    oven.setGraphLimits(setTemp + 25, setTimeMins);
                } else { // If timer is off, set graph limits to 25C above setTemp
                    oven.setGraphLimits(setTemp + 25, 60); // 1 minute for off state
                }
            }
            delay(200); // debounce
        }

        // If in edit mode, handle rotary changes
        if (editMode == TEMP) {
            long currentEncoder = rotaryEncoder.readEncoder();
            if (currentEncoder != lastEncoder) {
                int delta = (int)-(currentEncoder - lastEncoder);
                setTemp += delta; // 1 degree per detent
                if (setTemp < 0) setTemp = 0;
                if (setTemp > 250) setTemp = 250;
                lastEncoder = currentEncoder;
            }
        } else if (editMode == TIME) {
            long currentEncoder = rotaryEncoder.readEncoder();
            if (currentEncoder != lastEncoder) {
                int delta = (int)-(currentEncoder - lastEncoder);
                setTimeMins += delta; // 1 min per detent
                if (setTimeMins < 0) setTimeMins = 0; // Off
                if (setTimeMins > 720) setTimeMins = 720; // 12 hours
                lastEncoder = currentEncoder;
            }
        }
        // Add a break condition if you want to exit the oven cycle
        // For example, break if a button is long pressed or timer expired
        //if ((millis() - oven.startTimeMs) > setTimeMins*60000UL && setTimeMins > 0) break;
    }
}

void loop() {
  static char textBuffer[50];

  menuLoop(rotaryEncoder);
  gfx.setTextColor(TFT_BLUE, TFT_BLACK);
  gfx.setTextFont(1);
  gfx.setTextSize(1);
  gfx.setTextDatum(BR_DATUM);
  snprintf(textBuffer, sizeof(textBuffer), "  %.0fC", GetFilteredTemp(ReadTemp(true)));
  gfx.drawString(textBuffer, 159, 127);
 
//  gfx.printf("  %.0fC", GetFilteredTemp(ReadTemp(true)));
  gfx.setTextFont(2);
  gfx.setTextDatum(TL_DATUM);
  
}

