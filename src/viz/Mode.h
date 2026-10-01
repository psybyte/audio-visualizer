#pragma once

#include "analysis/Analyzer.h"
#include "Types.h"

#include <memory>
#include <vector>

class Batch;
class Shader;
class Trail;
class ScreenQuad;

struct VizContext {
  int width = 1;
  int height = 1;
  float time = 0.f;
  float dt = 0.016f;
  const AnalysisSnapshot* audio = nullptr;
  Palette palette = Palette::Neon;
  Batch* batch = nullptr;
  Trail* trail = nullptr;
  ScreenQuad* quad = nullptr;
  const Shader* basic = nullptr;
  const Shader* spectrogram = nullptr;
  const Shader* terrain = nullptr;
  const Shader* particle = nullptr;
  const Shader* kaleidoscope = nullptr;
};

class Mode {
 public:
  virtual ~Mode() = default;
  virtual const char* name() const = 0;
  virtual void draw(const VizContext& ctx) = 0;
  virtual void reset() {}
};

std::unique_ptr<Mode> createSpectrumBars();
std::unique_ptr<Mode> createRadialRing();
std::unique_ptr<Mode> createOscilloscope();
std::unique_ptr<Mode> createSpectrogram();
std::unique_ptr<Mode> createTerrain();
std::unique_ptr<Mode> createParticles();
std::unique_ptr<Mode> createLissajous();
std::unique_ptr<Mode> createKaleidoscope();

inline std::vector<std::unique_ptr<Mode>> createAllModes() {
  std::vector<std::unique_ptr<Mode>> modes;
  modes.reserve(kModeCount);
  modes.push_back(createSpectrumBars());
  modes.push_back(createRadialRing());
  modes.push_back(createOscilloscope());
  modes.push_back(createSpectrogram());
  modes.push_back(createTerrain());
  modes.push_back(createParticles());
  modes.push_back(createLissajous());
  modes.push_back(createKaleidoscope());
  return modes;
}
