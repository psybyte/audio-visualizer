#pragma once

#include "Types.h"

#include <string>

struct AnalysisSnapshot {
  static constexpr int kFftSize = 4096;
  static constexpr int kBinCount = kFftSize / 2;
  static constexpr int kBandCount = 128;
  static constexpr int kWaveCount = 2048;

  float waveL[kWaveCount]{};
  float waveR[kWaveCount]{};
  float bins[kBinCount]{};
  float bands[kBandCount]{};
  float peaks[kBandCount]{};
  float sampleRate = 48000.f;
  float bass = 0.f;
  float mid = 0.f;
  float treble = 0.f;
  float energy = 0.f;
};

class Analyzer {
 public:
  Analyzer();
  void reset();
  void process(const float* interleavedStereo, int sampleRate, float dt, const VisualSettings& settings, AnalysisSnapshot& out);
  static bool selfTest(std::string& error);

 private:
  float real_[AnalysisSnapshot::kFftSize]{};
  float imag_[AnalysisSnapshot::kFftSize]{};
  float hann_[AnalysisSnapshot::kFftSize]{};
  float smoothed_[AnalysisSnapshot::kBandCount]{};
  float peaks_[AnalysisSnapshot::kBandCount]{};
  float gain_ = 1.f;
  float waveGain_ = 1.f;
};
