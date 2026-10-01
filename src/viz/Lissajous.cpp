#include "viz/Mode.h"

#include "gl/Batch.h"
#include "gl/GlExt.h"
#include "gl/Shader.h"
#include "viz/Palette.h"
#include "viz/Trail.h"

#include <algorithm>
#include <cmath>

namespace {

class LissajousMode final : public Mode {
 public:
  const char* name() const override { return kModeNames[6]; }

  void draw(const VizContext& ctx) override {
    if (ctx.trail == nullptr || ctx.batch == nullptr || ctx.basic == nullptr || ctx.audio == nullptr) {
      return;
    }
    const float fade = std::exp(-std::max(ctx.dt, 0.f) * 5.5f);
    ctx.trail->begin(ctx.width, ctx.height, fade);
    ctx.batch->clear();
    const float cx = static_cast<float>(ctx.width) * 0.5f;
    const float cy = static_cast<float>(ctx.height) * 0.5f;
    const float scale = static_cast<float>(std::min(ctx.width, ctx.height)) * 0.38f;
    constexpr int kCount = AnalysisSnapshot::kWaveCount;
    constexpr int kStep = 2;
    float previousX = 0.f;
    float previousY = 0.f;
    bool hasPrevious = false;
    for (int i = 0; i < kCount; i += kStep) {
      const float x = cx + ctx.audio->waveL[i] * scale;
      const float y = cy + ctx.audio->waveR[i] * scale;
      const float t = static_cast<float>(i) / static_cast<float>(kCount - 1);
      const Rgb color = paletteColor(ctx.palette, t);
      if (hasPrevious) {
        ctx.batch->line(previousX, previousY, x, y, 5.5f, rgba(color, 0.14f));
        ctx.batch->line(previousX, previousY, x, y, 1.7f, rgba(scaleRgb(color, 1.15f), 0.9f));
      }
      previousX = x;
      previousY = y;
      hasPrevious = true;
    }
    ctx.batch->draw(*ctx.basic, static_cast<float>(ctx.width), static_cast<float>(ctx.height));
    ctx.batch->clear();
    ctx.trail->end(ctx.width, ctx.height);

    const Rgba axis = rgba(paletteColor(ctx.palette, 0.55f), 0.18f);
    ctx.batch->line(cx - scale, cy, cx + scale, cy, 1.f, axis);
    ctx.batch->line(cx, cy - scale, cx, cy + scale, 1.f, axis);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ctx.batch->draw(*ctx.basic, static_cast<float>(ctx.width), static_cast<float>(ctx.height));
    glDisable(GL_BLEND);
    ctx.batch->clear();
  }
};

}  // namespace

std::unique_ptr<Mode> createLissajous() {
  return std::make_unique<LissajousMode>();
}
