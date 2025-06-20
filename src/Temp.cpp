#include "max6675.h"
#include "DWFilter.h" 
#include <SPI.h>
#include <PID_v2.h>
#include "temp.h"
#include "ArduinoMenu.h"

int thermoDO = 39;
int thermoCS = 32;
int thermoCLK = 33;

DWFilter myFilter(3); 
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// double Kp = 1.7, Ki = 0.075, Kd = 55;
//double Kp = 1.7, Ki = 0.05, Kd = 35;
//double Kp = 1.9, Ki = 0.05, Kd = 35;
//double Kp = 2.5, Ki = 0.05, Kd = 25; // Overshooting
//double Kp = 2.5, Ki = 0.05, Kd = 35; 
// double Kp = 3, Ki = 0.05, Kd = 35; // Overshooting slightly. Decrease Kp to 2.75
//double Kp = 2.75, Ki = 0.05, Kd = 35; // Overshooting 
//double Kp = 2.8, Ki = 0.05, Kd = 40;
//double Kp = 2.0, Ki = 0.025, Kd = 45; // very good. But doesn't ramp well at peak temp
//double Kp = 2.5, Ki = 0.025, Kd = 45;
//double Kp = 2.5, Ki = 0.030, Kd = 55;
//double Kp = 2.5, Ki = 0.030, Kd = 65; 11 Jun 8:53
// double Kp = 2.5, Ki = 0.030, Kd = 75; 9:16pm
// double Kp = 2.0, Ki = 0.030, Kd = 75; // 12 Jun 7:56 Oscillations much better. Increase feed forward advance
//double Kp = 2.0, Ki = 0.030, Kd = 75; 
double Kp = 2.0, Ki = 0.05, Kd = 75; // Experiment with Ki increase



PID_v2 myPID(Kp, Ki, Kd, PID::Direct);

float currentReading;
unsigned long lastReadTime = 0L;


float ReadTemp(bool block) {
    unsigned long elapsedTime = millis() - lastReadTime;
    bool readNow = false;

    if( elapsedTime > 250) {
        // If not blocking and last read was more than 250ms ago, read again
        readNow = true;
    } else if( block ) {
        delay(250);
        readNow = true;
    }

    if( readNow ) {
        currentReading = GetFilteredTemp(thermocouple.readCelsius());
        lastReadTime = millis();
        // serial print the temp and the lastreadtime     
        //Serial.printf("Temp: %.2fC, Time: %lu ms\n", currentReading, lastReadTime);
    } 

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

// PID output functions
void InitPID() {
    myPID.SetOutputLimits(-100, 100);
    myPID.SetSampleTime(4000);
    float currentReading = myFilter.filter(ReadTemp(true));
    myPID.Start(currentReading, 0, currentReading);
}

void SetPIDTargetTemp(float temp) {
    myPID.Setpoint(temp);
}

float GetPIDOutput(float actualTemp) {  
    float output = myPID.Run(actualTemp);
    return output;
}


