#pragma once

float ReadTemp(bool block = false);
float GetFilteredTemp(float temp);
float Median3(float a, float b, float c);

// New methods
void InitPID();
void SetPIDTargetTemp(float temp);
float GetPIDOutput(float actualTemp);