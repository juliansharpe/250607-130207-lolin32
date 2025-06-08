#include "max6675.h"
#include "DWFilter.h" 
#include <SPI.h>
#include <PID_v2.h>
#include "temp.h"

int thermoDO = 39;
int thermoCS = 32;
int thermoCLK = 33;

const int fan =           25;
const int fryerElement =  26;
const int mainElement =   27;

DWFilter myFilter(3); 
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// ki 0.0025, Alpha = .1
double Kp = 1.75, Ki = 0.025, Kd = 45;
PID_v2 myPID(Kp, Ki, Kd, PID::Direct);

float currentReading;
unsigned long lastReadTime = 0L;

void TempInit() {
    myPID.SetOutputLimits(0, 100);
    myPID.SetSampleTime(4000);
    float currentReading = myFilter.filter(thermocouple.readCelsius());
    myPID.Start(currentReading, 0, 200);
}

float ReadTemp(bool block) {
    delay(250);
    lastReadTime = millis();
    currentReading = GetFilteredTemp(thermocouple.readCelsius());
    return currentReading;
}

float GetFilteredTemp(float temp) {
    static const int tempBufferSize = 3;
    static float tempBuffer[tempBufferSize];
    static int8_t tempPtr = -1; // -1 designates not initialised
    static float movingAverage=-1.0;    

    // Add to buffer
    if( tempPtr == -1 ) {
        // First reading
        for(uint i=0; i< tempBufferSize; i++) {
        tempBuffer[i] = temp;
        }
        tempPtr++;
    } else {
        tempPtr++;
        tempPtr%=tempBufferSize;
        tempBuffer[tempPtr] = temp;
    }

    float median = Median3( tempBuffer[0], tempBuffer[1], tempBuffer[2]);
    
    if( movingAverage < 0.0) {
        movingAverage = median;
    }

    movingAverage = 0.05 * median + (1 - 0.05) * movingAverage;
    return movingAverage;
}

float Median3(float a, float b, float c) {
    if ((a > b) != (a > c)) return a;
    else if ((b > a) != (b > c)) return b;
    else return c;
}
