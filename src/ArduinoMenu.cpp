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
  params.phases[0] = {"Preheat", 0, (float)profile.preheatTemp, 140000, 140000, false};
  params.phases[1] = {"Soak", (float)profile.preheatTemp, (float)profile.soakTemp, 120000, 120000, false};
  params.phases[2] = {"Peak", (float)profile.soakTemp, (float)profile.peakTemp, 70000, 120000, false};
  params.phases[3] = {"Dwell", (float)profile.peakTemp, (float)profile.peakTemp, (uint32_t)profile.dwellTime*1000, (uint32_t)profile.dwellTime*1000, false};
  params.phases[4] = {"Cool", (float)profile.peakTemp, 0, 90000, 90000, false};
  params.numPhases = 5;

  solderProfile.setProfile(params);

  Serial.printf("Starting reflow profile: Preheat %dC, Soak %dC, Peak %dC, Dwell %ds\n",
                profile.preheatTemp, profile.soakTemp, profile.peakTemp, profile.dwellTime);
  
  gfx.fillScreen(Black);
  gfx.setTextColor(Blue,Black);
  gfx.setTextSize(1);
  gfx.setTextFont(0);

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
      const float maxHeatRate = 100.0 / 100.0; // 100 degrees in 100 seconds

      uint32_t nowMs = millis();
      float feedForwardSlope = solderProfile.getFeedForwardSlope(10000); // seconds for feed-forward slope
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
    if (setTimeMins == -1) {
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

void StartOven() {
    Oven oven;
    gfx.fillScreen(Black);
    gfx.setTextColor(Blue, Black);
    gfx.setTextFont(1);
    gfx.setTextSize(1);

    float temp = 0;
    float setTemp = 0;
    int setTimeMins = 15; 
    //oven.setGraphLimits(setTemp + 25, setTimeMins);
    oven.initGraph(gfx, 0, 14, GFX_WIDTH-1, GFX_HEIGHT-14, 50.0, 5);
    unsigned long readtime = millis();
  
    enum EditMode { NONE, TEMP, TIME };
    EditMode editMode = NONE;
    long lastEncoder = rotaryEncoder.readEncoder();

    // --- PID and ElementPWM setup ---
    InitPID();
    ElementPWM elementPWM(mainElement, fryerElement, 1000); // 1Hz PWM
    uint8_t PWMMain = 0;
    uint8_t PWMFryer = 0;

    digitalWrite(fan, 1); // Turn on the fan

    unsigned long timerEndMs = 0;
    bool timerActive = true;
    timerEndMs = millis() + (unsigned long)setTimeMins * 60000UL;

    while (true) {
        unsigned long now = millis();
        unsigned long elapsed = now - readtime;
        unsigned long msLeft = 0;
        int minsLeft = 0;
        if (timerActive) {
            msLeft = (timerEndMs > now) ? (timerEndMs - now) : 0;
            minsLeft = (msLeft + 30000) / 60000; // Round up to nearest minute
        } else {
            msLeft = 0;
            minsLeft = 0;
        }

        if (elapsed > 250) {
            readtime = now;
            temp = GetFilteredTemp(ReadTemp(false));

            // --- PID control logic ---
            SetPIDTargetTemp(setTemp);
            float pidOutput = GetPIDOutput(temp);
            pidOutput = constrain(pidOutput, 0, 100);

            if (pidOutput > 50) {
                PWMMain = 100;
                PWMFryer = constrain((pidOutput - 50) * 2, 0, 100);
            } else {
                PWMMain = constrain(pidOutput * 2, 0, 100);
                PWMFryer = 0;
            }
            elementPWM.setPWM(PWMMain, PWMFryer);

            // Write out to the serial monitor the temp, settemp, pid output, and PID components
            if (timerActive && msLeft == 0) {
                Serial.println("Timer expired, stopping heat.");
                timerActive = false;
                elementPWM.setPWM(0, 0);
                elementPWM.process();
                break; // Exit the loop if timer expired
            }
            // Serial.printf(
            //     "Temp: %.1fC, SetTemp: %.1fC, PID Output: %.0f, Timer: %d mins | P:%.0f I:%.0f D:%.0f\n",
            //     temp, setTemp, pidOutput, minsLeft,
            //     myPID.GetLastP(), myPID.GetLastI(), myPID.GetLastD()
            //);
        }
        oven.updateGraph(temp, setTemp);
        // Regularly update the PWM outputs
        elementPWM.process();

        gfx.setTextSize(1);
        gfx.setCursor(0, 0);
        WriteTemp(temp, setTemp, editMode == TEMP);
        if( editMode == TIME) {
          WriteTime(setTimeMins, true);
        } else {
          WriteTime(minsLeft, false);
        }

        // Handle rotary button click to cycle edit modes
        if (rotaryEncoder.isEncoderButtonClicked()) {
            if (editMode == NONE) {
                editMode = TEMP;
            } else if (editMode == TEMP) {
                editMode = TIME;
            } else if (editMode == TIME) {
                editMode = NONE;
                // When timer is set, start/restart countdown from now
                if (setTimeMins > 0) {
                    unsigned long finishMins = (elapsed / 60000UL) + setTimeMins;
                    //oven.setGraphLimits(setTemp + 50, (elapsed / 60000UL) + 1);
                    timerEndMs = millis() + (finishMins * 60000UL);
                    timerActive = true;
                } else { // If timer is off, set graph limits to 25C above setTemp
                    //oven.setGraphLimits(setTemp + 50, 1); 
                    timerActive = false;
                }
                oven.redrawGraph();
                InitPID(); // Reinitialize PID for new settings
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
                if (setTimeMins < -1) setTimeMins = -1; // Off
                if (setTimeMins > 720) setTimeMins = 720; // 12 hours
                lastEncoder = currentEncoder;
            }
        }

        // Break if timer is active and expired
        if (timerActive && msLeft == 0) {
            Serial.println("Oven timer expired. Stopping heat.");
            elementPWM.setPWM(0, 0);
            elementPWM.process();
            digitalWrite(mainElement, 0); // Turn off the main element
            digitalWrite(fryerElement, 0); // Turn off the fryer element  
            
            gfx.setTextColor(TFT_BLUE, TFT_BLACK);
            gfx.setCursor(0, 0);
            gfx.printf("Finished                   \n");
            unsigned long finishTime = millis();
            while(!rotaryEncoder.isEncoderButtonClicked())
            {
              if( millis() - finishTime > 10000) {
                digitalWrite(fan, 0); // Turn off the fan
              }
              delay(100);
            }
            break; 
        }
        // Optionally, add other break conditions (e.g., button long press)
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

