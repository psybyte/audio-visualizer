#include "viz/Mode.h"

#include "gl/GlExt.h"
#include "gl/Shader.h"
#include "viz/DrawUtil.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265f;

class HelixMode final : public Mode {
 public:
  const char* name() const override { return "Helix"; }

  void draw(const VizContext& ctx) override {
    if (ctx.width < 2 || ctx.audio == nullptr || ctx.batch == nullptr || ctx.basic == nullptr) {
      return;
    }
    const float width = static_cast<float>(ctx.width);
    const float height = static_cast<float>(ctx.height);
    const float cx = width * 0.5f;
    const float cy = height * 0.52f;
    const float radius = std::min(width, height) * 0.22f;
    constexpr int kCount = AnalysisSnapshot::kWaveCount;
    constexpr int kStep = 6;
    constexpr int kBands = AnalysisSnapshot::kBandCount;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    ctx.batch->clear();
    queueBackground(*ctx.batch, ctx.width, ctx.height, ctx.palette);
    ctx.batch->draw(*ctx.basic, width, height);
    ctx.batch->clear();

    auto strand = [&](float phase, float toneShift) {
      float previousX = 0.f;
      float previousY = 0.f;
      bool hasPrevious = false;
      for (int i = 0; i < kCount; i += kStep) {
        const float z = static_cast<float>(i) / static_cast<float>(kCount - 1);
        const float depth = 0.28f + 0.85f * (1.f - z);
        const float sample = phase < 1.f ? ctx.audio->waveL[i] : ctx.audio->waveR[i];
        const int band = std::min(kBands - 1, static_cast<int>(z * static_cast<float>(kBands - 1)));
        const float amp = std::clamp(ctx.audio->bands[band], 0.f, 1.f);
        const float angle = z * kPi * 8.f + ctx.time * 0.8f + phase + sample * 0.9f;
        const float x = cx + std::cos(angle) * radius * depth * (0.75f + amp * 0.55f);
        const float y = cy + std::sin(angle) * radius * 0.55f * depth + (0.5f - z) * height * 0.62f;
        const Rgb color = paletteColor(ctx.palette, std::clamp(z * 0.8f + toneShift, 0.f, 1.f));
        const float size = (1.4f + amp * 5.5f) * depth;
        ctx.batch->disc(x, y, size, rgba(color, 0.2f + amp * 0.75f), 8);
        if (hasPrevious) {
          ctx.batch->line(previousX, previousY, x, y, 1.2f * depth, rgba(color, 0.18f + amp * 0.35f));
        }
        previousX = x;
        previousY = y;
        hasPrevious = true;
      }
    };
    strand(0.f, 0.05f);
    strand(kPi, 0.45f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    ctx.batch->draw(*ctx.basic, width, height);
    glDisable(GL_BLEND);
    ctx.batch->clear();
  }
};

}  // namespace

std::unique_ptr<Mode> createHelix() {
  return std::make_unique<HelixMode>();
}
