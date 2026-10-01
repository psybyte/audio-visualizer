#pragma once

#include "gl/Batch.h"
#include "viz/Palette.h"

inline void queueBackground(Batch& batch, int width, int height, Palette palette) {
  batch.rectVertical(0.f, 0.f, static_cast<float>(width), static_cast<float>(height), rgba(backgroundBottom(palette)), rgba(backgroundTop(palette)));
}
