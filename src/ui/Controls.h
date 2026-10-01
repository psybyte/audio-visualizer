#pragma once

#include "Types.h"

#include <functional>
#include <string>
#include <vector>

struct UiState {
  AudioSource source = AudioSource::Tone;
  int mode = 0;
  float sensitivity = 1.45f;
  float smoothing = 0.58f;
  Palette palette = Palette::Neon;
  bool showPanel = true;
  std::string fileName;
  std::string error;
  int sampleRate = 0;
  float fps = 0.f;
  bool filePlaying = false;
  float filePosition = 0.f;
  float fileDuration = 0.f;
  std::vector<std::string> captureDevices;
  int captureDeviceIndex = -1;
  std::string captureDeviceName;
};

struct UiActions {
  std::function<void(AudioSource)> selectSource;
  std::function<void()> browse;
  std::function<void(bool)> setPlaying;
  std::function<void(float)> seek;
  std::function<void()> refreshCaptureDevices;
  std::function<void(int)> selectCaptureDevice;
};

void drawControls(UiState& state, const UiActions& actions);
