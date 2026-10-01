#include "analysis/Analyzer.h"

#include "analysis/Fft.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979323846f;

float clamp01(float value) {
  return std::clamp(value, 0.f, 1.f);
}

float normalizeMagnitude(float magnitude, float gain) {
  const float db = 20.f * std::log10(magnitude + 1e-8f);
  return clamp01((db + 72.f) / 72.f * gain);
}

float bandCenterHz(int band, float sampleRate) {
  const float fMin = 20.f;
  const float fMax = std::max(2000.f, sampleRate * 0.5f);
  const float t = (static_cast<float>(band) + 0.5f) / static_cast<float>(AnalysisSnapshot::kBandCount);
  return fMin * std::pow(fMax / fMin, t);
}

}  // namespace

Analyzer::Analyzer() {
  for (int i = 0; i < AnalysisSnapshot::kFftSize; ++i) {
    hann_[i] = 0.5f * (1.f - std::cos(2.f * kPi * static_cast<float>(i) / static_cast<float>(AnalysisSnapshot::kFftSize - 1)));
  }
}

void Analyzer::reset() {
  std::fill(std::begin(smoothed_), std::end(smoothed_), 0.f);
  std::fill(std::begin(peaks_), std::end(peaks_), 0.f);
  gain_ = 1.f;
  waveGain_ = 1.f;
}

void Analyzer::process(const float* interleavedStereo, int sampleRate, float dt, const VisualSettings& settings, AnalysisSnapshot& out) {
  dt = std::clamp(dt, 0.f, 0.05f);
  const int fftSize = AnalysisSnapshot::kFftSize;
  const int binCount = AnalysisSnapshot::kBinCount;
  const int bandCount = AnalysisSnapshot::kBandCount;
  const float rate = sampleRate > 0 ? static_cast<float>(sampleRate) : 48000.f;
  out.sampleRate = rate;

  float wavePeak = 0.f;
  for (int i = 0; i < fftSize; ++i) {
    float left = interleavedStereo[i * 2];
    float right = interleavedStereo[i * 2 + 1];
    if (!std::isfinite(left)) {
      left = 0.f;
    }
    if (!std::isfinite(right)) {
      right = 0.f;
    }
    wavePeak = std::max(wavePeak, std::max(std::fabs(left), std::fabs(right)));
    const float mono = 0.5f * (left + right);
    real_[i] = mono * hann_[i];
    imag_[i] = 0.f;
  }

  const float waveTarget = std::clamp(0.9f / std::max(wavePeak, 0.02f), 0.5f, 10.f);
  const float waveFollow = 1.f - std::exp(-dt * 6.f);
  waveGain_ += (waveTarget - waveGain_) * waveFollow;
  const int waveCount = AnalysisSnapshot::kWaveCount;
  const int stride = fftSize / waveCount;
  for (int i = 0; i < waveCount; ++i) {
    const int sample = i * stride;
    float left = interleavedStereo[sample * 2];
    float right = interleavedStereo[sample * 2 + 1];
    if (!std::isfinite(left)) {
      left = 0.f;
    }
    if (!std::isfinite(right)) {
      right = 0.f;
    }
    out.waveL[i] = std::clamp(left * waveGain_, -1.2f, 1.2f);
    out.waveR[i] = std::clamp(right * waveGain_, -1.2f, 1.2f);
  }

  fftRadix2(real_, imag_, fftSize);

  float maxMagnitude = 0.f;
  float magnitudes[AnalysisSnapshot::kBinCount];
  for (int i = 0; i < binCount; ++i) {
    const float scale = (i == 0) ? (1.f / static_cast<float>(fftSize)) : (2.f / static_cast<float>(fftSize));
    const float magnitude = std::sqrt(real_[i] * real_[i] + imag_[i] * imag_[i]) * scale;
    magnitudes[i] = magnitude;
    if (i > 0) {
      maxMagnitude = std::max(maxMagnitude, magnitude);
    }
  }

  float targetGain = 1.f;
  if (maxMagnitude >= 1e-4f) {
    const float current = std::max((20.f * std::log10(maxMagnitude + 1e-8f) + 72.f) / 72.f, 0.12f);
    targetGain = std::clamp(0.86f / current, 0.35f, 5.5f);
  }
  targetGain *= std::clamp(settings.sensitivity, 0.15f, 4.f);
  const float gainFollow = 1.f - std::exp(-dt * 3.5f);
  gain_ += (targetGain - gain_) * (dt <= 0.f ? 1.f : gainFollow);

  for (int i = 0; i < binCount; ++i) {
    float value = normalizeMagnitude(magnitudes[static_cast<size_t>(i)], gain_);
    if (value < 0.045f) {
      value = 0.f;
    }
    out.bins[i] = value;
  }

  const float nyquist = rate * 0.5f;
  const float fMin = 20.f;
  const float fMax = std::max(fMin + 1.f, nyquist);
  float rawBands[AnalysisSnapshot::kBandCount]{};
  for (int band = 0; band < bandCount; ++band) {
    const float t0 = static_cast<float>(band) / static_cast<float>(bandCount);
    const float t1 = static_cast<float>(band + 1) / static_cast<float>(bandCount);
    const float hz0 = fMin * std::pow(fMax / fMin, t0);
    const float hz1 = fMin * std::pow(fMax / fMin, t1);
    int i0 = static_cast<int>(hz0 * static_cast<float>(fftSize) / rate);
    int i1 = static_cast<int>(hz1 * static_cast<float>(fftSize) / rate);
    i0 = std::clamp(i0, 1, binCount - 1);
    i1 = std::clamp(i1, i0 + 1, binCount);
    float peak = 0.f;
    float sum = 0.f;
    for (int i = i0; i < i1; ++i) {
      peak = std::max(peak, out.bins[i]);
      sum += out.bins[i];
    }
    const float mean = sum / static_cast<float>(i1 - i0);
    rawBands[band] = clamp01(peak * 0.7f + mean * 0.3f);
  }

  const float smooth = std::clamp(settings.smoothing, 0.f, 1.f);
  const float attack = 1.f - std::exp(-dt * (16.f + (1.f - smooth) * 46.f));
  const float release = 1.f - std::exp(-dt * (1.6f + (1.f - smooth) * 18.f));
  const float peakFall = std::exp(-dt * 0.75f);
  float energy = 0.f;
  float bass = 0.f;
  float mid = 0.f;
  float treble = 0.f;
  int bassCount = 0;
  int midCount = 0;
  int trebleCount = 0;
  for (int band = 0; band < bandCount; ++band) {
    const float coefficient = rawBands[band] > smoothed_[band] ? attack : release;
    smoothed_[band] += (rawBands[band] - smoothed_[band]) * (dt <= 0.f ? 1.f : coefficient);
    if (smoothed_[band] >= peaks_[band]) {
      peaks_[band] = smoothed_[band];
    } else {
      peaks_[band] *= peakFall;
    }
    out.bands[band] = smoothed_[band];
    out.peaks[band] = peaks_[band];
    energy += smoothed_[band];
    const float position = static_cast<float>(band) / static_cast<float>(bandCount);
    if (position < 0.18f) {
      bass += smoothed_[band];
      ++bassCount;
    } else if (position < 0.62f) {
      mid += smoothed_[band];
      ++midCount;
    } else {
      treble += smoothed_[band];
      ++trebleCount;
    }
  }
  out.energy = energy / static_cast<float>(bandCount);
  out.bass = bassCount > 0 ? bass / static_cast<float>(bassCount) : 0.f;
  out.mid = midCount > 0 ? mid / static_cast<float>(midCount) : 0.f;
  out.treble = trebleCount > 0 ? treble / static_cast<float>(trebleCount) : 0.f;
}

bool Analyzer::selfTest(std::string& error) {
  if (!fftSelfTest(error)) {
    return false;
  }
  constexpr int kSize = AnalysisSnapshot::kFftSize;
  constexpr int kRate = 48000;
  constexpr int kBin = 86;
  const float frequency = static_cast<float>(kBin) * static_cast<float>(kRate) / static_cast<float>(kSize);
  std::vector<float> stereo(static_cast<size_t>(kSize) * 2, 0.f);
  for (int i = 0; i < kSize; ++i) {
    const float sample = 0.7f * std::sin(2.f * kPi * frequency * static_cast<float>(i) / static_cast<float>(kRate));
    stereo[static_cast<size_t>(i) * 2] = sample;
    stereo[static_cast<size_t>(i) * 2 + 1] = sample * 0.85f;
  }

  Analyzer analyzer;
  AnalysisSnapshot snapshot{};
  VisualSettings settings;
  settings.sensitivity = 1.f;
  settings.smoothing = 0.2f;
  for (int frame = 0; frame < 24; ++frame) {
    analyzer.process(stereo.data(), kRate, 1.f / 60.f, settings, snapshot);
  }

  int loudest = 0;
  for (int band = 1; band < AnalysisSnapshot::kBandCount; ++band) {
    if (snapshot.bands[band] > snapshot.bands[loudest]) {
      loudest = band;
    }
  }
  const float center = bandCenterHz(loudest, static_cast<float>(kRate));
  const float ratio = center / frequency;
  if (snapshot.bands[loudest] < 0.25f || ratio < 0.62f || ratio > 1.62f) {
    std::ostringstream stream;
    stream << "The " << frequency << " Hz sine landed in band " << loudest << " (" << center
           << " Hz, level " << snapshot.bands[loudest] << ").";
    error = stream.str();
    return false;
  }
  return true;
}
