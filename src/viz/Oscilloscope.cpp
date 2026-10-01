#include "viz/Mode.h"

#include "gl/Batch.h"
#include "gl/GlExt.h"
#include "gl/Shader.h"
#include "viz/Palette.h"
#include "viz/Trail.h"

#include <algorithm>
#include <cmath>

namespace {

void drawWave(Batch& batch, const AnalysisSnapshot& audio, int width, int height, bool rightChannel, float thickness, Rgba color) {
  const float mid = static_cast<float>(height) * (rightChannel ? 0.5f : 0.5f);
  const float amp = static_cast<float>(height) * (rightChannel ? 0.30f : 0.40f);
  const float* samples = rightChannel ? audio.waveR : audio.waveL;
  constexpr int kCount = AnalysisSnapshot::kWaveCount;
  constexpr int kStep = 2;
  for (int i = 0; i + kStep < kCount; i += kStep) {
    const float x0 = static_cast<float>(i) / static_cast<float>(kCount - 1) * static_cast<float>(width);
    const float x1 = static_cast<float>(i + kStep) / static_cast<float>(kCount - 1) * static_cast<float>(width);
    const float y0 = mid + samples[i] * amp;
    const float y1 = mid + samples[i + kStep] * amp;
    batch.line(x0, y0, x1, y1, thickness, color);
  }
}

class Oscilloscope final : public Mode {
 public:
  const char* name() const override { return kModeNames[2]; }

  void draw(const VizContext& ctx) override {
    if (ctx.trail == nullptr || ctx.batch == nullptr || ctx.basic == nullptr || ctx.audio == nullptr) {
      return;
    }
    const float fade = std::exp(-std::max(ctx.dt, 0.f) * 7.2f);
    ctx.trail->begin(ctx.width, ctx.height, fade);
    ctx.batch->clear();
    const Rgb tint = paletteColor(ctx.palette, 0.72f);
    drawWave(*ctx.batch, *ctx.audio, ctx.width, ctx.height, true, 7.f, rgba(tint, 0.12f));
    drawWave(*ctx.batch, *ctx.audio, ctx.width, ctx.height, false, 6.5f, rgba(tint, 0.22f));
    drawWave(*ctx.batch, *ctx.audio, ctx.width, ctx.height, false, 1.8f, rgba(scaleRgb(paletteColor(ctx.palette, 0.92f), 1.2f), 0.95f));
    ctx.batch->draw(*ctx.basic, static_cast<float>(ctx.width), static_cast<float>(ctx.height));
    ctx.batch->clear();
    ctx.trail->end(ctx.width, ctx.height);

    const Rgba grid = rgba(paletteColor(ctx.palette, 0.45f), 0.14f);
    const float width = static_cast<float>(ctx.width);
    const float height = static_cast<float>(ctx.height);
    for (int i = 1; i < 8; ++i) {
      const float y = height * static_cast<float>(i) / 8.f;
      ctx.batch->line(0.f, y, width, y, 1.f, grid);
    }
    ctx.batch->line(0.f, height * 0.5f, width, height * 0.5f, 1.4f, rgba(paletteColor(ctx.palette, 0.8f), 0.28f));
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ctx.batch->draw(*ctx.basic, width, height);
    glDisable(GL_BLEND);
    ctx.batch->clear();
  }
};

}  // namespace

std::unique_ptr<Mode> createOscilloscope() {
  return std::make_unique<Oscilloscope>();
}
