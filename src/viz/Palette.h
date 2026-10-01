#pragma once

#include "Types.h"

struct Rgb {
  float r = 0.f;
  float g = 0.f;
  float b = 0.f;
};

struct Rgba {
  float r = 0.f;
  float g = 0.f;
  float b = 0.f;
  float a = 1.f;
};

inline Rgba rgba(Rgb color, float alpha = 1.f) {
  return {color.r, color.g, color.b, alpha};
}

inline Rgb scaleRgb(Rgb color, float gain) {
  return {color.r * gain, color.g * gain, color.b * gain};
}

Rgb paletteColor(Palette palette, float t);
Rgb backgroundTop(Palette palette);
Rgb backgroundBottom(Palette palette);
