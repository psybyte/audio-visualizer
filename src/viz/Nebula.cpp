#include "viz/Mode.h"

#include "gl/GlExt.h"
#include "gl/Shader.h"
#include "viz/DrawUtil.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265f;

float starJitter(int index) {
  uint32_t value = static_cast<uint32_t>(index) * 747796405u + 2891336453u;
  value = (value >> 16) ^ value;
  return static_cast<float>(value & 0xFFFF) / 65535.f;
}

class NebulaMode final : public Mode {
 public:
  const char* name() const override { return "Nebula"; }

  void draw(const VizContext& ctx) override {
    if (ctx.width < 2 || ctx.audio == nullptr || ctx.batch == nullptr || ctx.basic == nullptr) {
      return;
    }
    const float width = static_cast<float>(ctx.width);
    const float height = static_cast<float>(ctx.height);
    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const float reach = std::min(width, height) * 0.46f;
    constexpr int kBands = AnalysisSnapshot::kBandCount;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    ctx.batch->clear();
    queueBackground(*ctx.batch, ctx.width, ctx.height, ctx.palette);
    ctx.batch->draw(*ctx.basic, width, height);
    ctx.batch->clear();

    const Rgb core = paletteColor(ctx.palette, 0.15f + ctx.audio->bass * 0.4f);
    ctx.batch->disc(cx, cy, reach * (0.08f + ctx.audio->bass * 0.16f), rgba(core, 0.22f + ctx.audio->bass * 0.45f), 48);

    for (int band = 0; band < kBands; ++band) {
      const float t = static_cast<float>(band) / static_cast<float>(kBands - 1);
      const float amp = std::clamp(ctx.audio->bands[band], 0.f, 1.f);
      const float arm = static_cast<float>(band % 3);
      const float angle = t * kPi * 7.5f + ctx.time * (0.12f + arm * 0.04f) + arm * 2.1f;
      const float radius = reach * (0.08f + t * 0.9f) * (0.55f + amp * 0.7f);
      const Rgb color = paletteColor(ctx.palette, t);
      for (int spark = 0; spark < 3; ++spark) {
        const float spin = angle + starJitter(band * 3 + spark) * 0.55f;
        const float distance = radius * (0.82f + starJitter(band * 5 + spark) * 0.28f);
        const float x = cx + std::cos(spin) * distance;
        const float y = cy + std::sin(spin) * distance * 0.72f;
        const float size = 1.6f + amp * (4.f + static_cast<float>(2 - spark) * 5.f);
        ctx.batch->disc(x, y, size, rgba(color, 0.18f + amp * 0.75f), 10);
      }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    ctx.batch->draw(*ctx.basic, width, height);
    glDisable(GL_BLEND);
    ctx.batch->clear();
  }
};

}  // namespace

std::unique_ptr<Mode> createNebula() {
  return std::make_unique<NebulaMode>();
}
