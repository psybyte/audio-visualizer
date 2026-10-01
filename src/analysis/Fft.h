#pragma once

#include <string>

void fftRadix2(float* real, float* imag, int size);
bool fftSelfTest(std::string& error);
