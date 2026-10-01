#include "viz/Mode.h"

#include "gl/GlExt.h"
#include "gl/Shader.h"
#include "viz/DrawUtil.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

constexpr float kPi = 3.14159265f;

struct Blip {
  float x = 0.f;
  float y = 0.f;
  float age = 0.f;
  float tone = 0.f;
  float strength = 0.f;
};

class RadarMode final : public Mode {
 public:
  const char* name() const override { return "Radar"; }

  void reset() override { blips_.clear(); }

  void draw(const VizContext& ctx) override {
    if (ctx.width < 2 || ctx.audio == nullptr || ctx.batch == nullptr || ctx.basic == nullptr) {
      return;
    }
    const float width = static_cast<float>(ctx.width);
    const float height = static_cast<float>(ctx.height);
    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const float radius = std::min(width, height) * 0.42f;
    const float dt = std::clamp(ctx.dt, 0.f, 0.05f);
    const float sweep = std::fmod(ctx.time * 0.65f, kPi * 2.f);
    constexpr int kBands = AnalysisSnapshot::kBandCount;

    for (Blip& blip : blips_) {
      blip.age += dt;
    }
    blips_.erase(std::remove_if(blips_.begin(), blips_.end(), [](const Blip& blip) { return blip.age > 3.2f; }), blips_.end());

    for (int band = 0; band < kBands; band += 2) {
      const float amp = std::clamp(ctx.audio->bands[band], 0.f, 1.f);
      if (amp < 0.04f) {
        continue;
      }
      const float t = static_cast<float>(band) / static_cast<float>(kBands - 1);
      const float distance = radius * (0.08f + t * 0.92f);
      Blip blip;
      blip.x = cx + std::cos(sweep) * distance;
      blip.y = cy + std::sin(sweep) * distance;
      blip.tone = t;
      blip.strength = amp;
      blips_.push_back(blip);
    }
    if (blips_.size() > 5000) {
      blips_.erase(blips_.begin(), blips_.begin() + static_cast<std::ptrdiff_t>(blips_.size() - 5000));
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    ctx.batch->clear();
    queueBackground(*ctx.batch, ctx.width, ctx.height, ctx.palette);
    ctx.batch->draw(*ctx.basic, width, height);
    ctx.batch->clear();

    const Rgba ring = rgba(paletteColor(ctx.palette, 0.45f), 0.28f);
    for (int circle = 1; circle <= 4; ++circle) {
      const float r = radius * static_cast<float>(circle) / 4.f;
      float previousX = cx + r;
      float previousY = cy;
      constexpr int kSteps = 72;
      for (int step = 1; step <= kSteps; ++step) {
        const float angle = static_cast<float>(step) / static_cast<float>(kSteps) * kPi * 2.f;
        const float x = cx + std::cos(angle) * r;
        const float y = cy + std::sin(angle) * r;
        ctx.batch->line(previousX, previousY, x, y, 1.2f, ring);
        previousX = x;
        previousY = y;
      }
    }
    ctx.batch->line(cx, cy, cx + std::cos(sweep) * radius, cy + std::sin(sweep) * radius, 2.2f, rgba(paletteColor(ctx.palette, 0.85f), 0.85f));

    for (const Blip& blip : blips_) {
      const float fade = 1.f - blip.age / 3.2f;
      const Rgb color = paletteColor(ctx.palette, blip.tone);
      ctx.batch->disc(blip.x, blip.y, 1.5f + blip.strength * 5.f, rgba(color, fade * (0.25f + blip.strength * 0.75f)), 8);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    ctx.batch->draw(*ctx.basic, width, height);
    glDisable(GL_BLEND);
    ctx.batch->clear();
  }

 private:
  std::vector<Blip> blips_;
};

}  // namespace

std::unique_ptr<Mode> createRadar() {
  return std::make_unique<RadarMode>();
}
