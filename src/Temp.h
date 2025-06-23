#pragma once
#include <PID_v2.h>

float ReadTemp(bool block = false);
float GetFilteredTemp(float temp);
float Median3(float a, float b, float c);
float Median5(float a, float b, float c, float d, float e);

// New methods
void InitPID();
void SetPIDTargetTemp(float temp);
float GetPIDOutput(float actualTemp);

// New method for sensor initialization
void InitTempSensor();

extern PID_v2 myPID;