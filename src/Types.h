#pragma once

enum class AudioSource {
  System = 0,
  Microphone,
  File,
  Tone,
};

enum class Palette {
  Neon = 0,
  Phosphor,
  Magma,
  Ice,
  Count,
};

inline constexpr const char* kModeNames[] = {
    "Bars",
    "Ring",
    "Oscilloscope",
    "Spectrogram",
    "Terrain",
    "Particles",
    "Lissajous",
    "Kaleidoscope",
};

inline constexpr const char* kPaletteNames[] = {
    "Neon",
    "Phosphor",
    "Magma",
    "Ice",
};

struct VisualSettings {
  float sensitivity = 1.45f;
  float smoothing = 0.58f;
  Palette palette = Palette::Neon;
};
