#pragma once

void TempInit();
float ReadTemp(bool block = true);
float GetFilteredTemp( float temp);
float Median3(float a, float b, float c);