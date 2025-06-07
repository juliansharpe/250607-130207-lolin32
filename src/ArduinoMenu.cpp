#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AiEsp32RotaryEncoder.h>
#include "MenuConfig.h"

// Function prototypes
void setup();
void loop();
void IRAM_ATTR readEncoderISR();

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

void loop() {
  menuLoop(rotaryEncoder);
}

