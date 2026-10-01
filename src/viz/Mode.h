#pragma once

#include "analysis/Analyzer.h"
#include "Types.h"

#include <memory>
#include <string>
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
std::unique_ptr<Mode> createNebula();
std::unique_ptr<Mode> createSkyline();
std::unique_ptr<Mode> createRadar();
std::unique_ptr<Mode> createRipples();
std::unique_ptr<Mode> createStereoField();
std::unique_ptr<Mode> createHelix();
std::unique_ptr<Mode> createBuiltinVisualization(const std::string& id);
