#include "viz/Palette.h"

#include <algorithm>

namespace {

float smooth(float t) {
  t = std::clamp(t, 0.f, 1.f);
  return t * t * (3.f - 2.f * t);
}

Rgb mix(Rgb a, Rgb b, float t) {
  const float u = smooth(t);
  return {a.r + (b.r - a.r) * u, a.g + (b.g - a.g) * u, a.b + (b.b - a.b) * u};
}

Rgb ramp(float t, Rgb c0, Rgb c1, Rgb c2, Rgb c3) {
  t = std::clamp(t, 0.f, 1.f);
  if (t < 0.35f) {
    return mix(c0, c1, t / 0.35f);
  }
  if (t < 0.70f) {
    return mix(c1, c2, (t - 0.35f) / 0.35f);
  }
  return mix(c2, c3, (t - 0.70f) / 0.30f);
}

}  // namespace

Rgb paletteColor(Palette palette, float t) {
  switch (palette) {
    case Palette::Phosphor:
      return ramp(t, {0.00f, 0.10f, 0.03f}, {0.05f, 0.55f, 0.16f}, {0.45f, 0.95f, 0.30f}, {0.80f, 1.00f, 0.62f});
    case Palette::Magma:
      return ramp(t, {0.08f, 0.00f, 0.03f}, {0.72f, 0.06f, 0.08f}, {1.00f, 0.42f, 0.05f}, {1.00f, 0.95f, 0.72f});
    case Palette::Ice:
      return ramp(t, {0.02f, 0.06f, 0.22f}, {0.10f, 0.32f, 0.90f}, {0.35f, 0.86f, 1.00f}, {0.92f, 1.00f, 1.00f});
    case Palette::Neon:
    case Palette::Count:
      break;
  }
  return ramp(t, {0.20f, 0.02f, 0.48f}, {0.95f, 0.10f, 0.55f}, {0.15f, 0.82f, 1.00f}, {0.88f, 1.00f, 1.00f});
}

Rgb backgroundTop(Palette palette) {
  switch (palette) {
    case Palette::Phosphor:
      return {0.01f, 0.06f, 0.03f};
    case Palette::Magma:
      return {0.10f, 0.02f, 0.03f};
    case Palette::Ice:
      return {0.02f, 0.06f, 0.14f};
    case Palette::Neon:
    case Palette::Count:
      break;
  }
  return {0.07f, 0.02f, 0.12f};
}

Rgb backgroundBottom(Palette palette) {
  switch (palette) {
    case Palette::Phosphor:
      return {0.00f, 0.015f, 0.01f};
    case Palette::Magma:
      return {0.03f, 0.00f, 0.01f};
    case Palette::Ice:
      return {0.00f, 0.015f, 0.04f};
    case Palette::Neon:
    case Palette::Count:
      break;
  }
  return {0.015f, 0.01f, 0.03f};
}
