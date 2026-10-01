#include "viz/Mode.h"

#include "gl/GlExt.h"
#include "gl/Shader.h"
#include "viz/DrawUtil.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

constexpr float kPi = 3.14159265f;

struct WaveRing {
  float radius = 0.f;
  float life = 1.f;
  float tone = 0.f;
  float strength = 0.f;
};

class RipplesMode final : public Mode {
 public:
  const char* name() const override { return "Ripples"; }

  void reset() override {
    rings_.clear();
    cooldown_ = 0.f;
  }

  void draw(const VizContext& ctx) override {
    if (ctx.width < 2 || ctx.audio == nullptr || ctx.batch == nullptr || ctx.basic == nullptr) {
      return;
    }
    const float width = static_cast<float>(ctx.width);
    const float height = static_cast<float>(ctx.height);
    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const float limit = std::min(width, height) * 0.52f;
    const float dt = std::clamp(ctx.dt, 0.f, 0.05f);
    cooldown_ = std::max(0.f, cooldown_ - dt);

    for (WaveRing& ring : rings_) {
      ring.radius += (90.f + ring.strength * 220.f) * dt;
      ring.life -= dt * (0.18f + ring.strength * 0.12f);
    }
    rings_.erase(std::remove_if(rings_.begin(), rings_.end(), [&](const WaveRing& ring) { return ring.life <= 0.f || ring.radius > limit * 1.3f; }),
                 rings_.end());

    if (cooldown_ <= 0.f && ctx.audio->energy > 0.08f) {
      int loudest = 0;
      for (int band = 1; band < AnalysisSnapshot::kBandCount; ++band) {
        if (ctx.audio->bands[band] > ctx.audio->bands[loudest]) {
          loudest = band;
        }
      }
      WaveRing ring;
      ring.tone = static_cast<float>(loudest) / static_cast<float>(AnalysisSnapshot::kBandCount - 1);
      ring.strength = std::clamp(ctx.audio->bands[loudest], 0.f, 1.f);
      ring.radius = 8.f + ctx.audio->bass * 20.f;
      rings_.push_back(ring);
      cooldown_ = 0.045f;
      if (rings_.size() > 48) {
        rings_.erase(rings_.begin());
      }
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    ctx.batch->clear();
    queueBackground(*ctx.batch, ctx.width, ctx.height, ctx.palette);
    ctx.batch->draw(*ctx.basic, width, height);
    ctx.batch->clear();

    ctx.batch->disc(cx, cy, 10.f + ctx.audio->bass * 28.f, rgba(paletteColor(ctx.palette, 0.2f), 0.35f + ctx.audio->bass * 0.5f), 28);
    for (const WaveRing& ring : rings_) {
      const Rgb color = paletteColor(ctx.palette, ring.tone);
      const float thickness = 1.5f + ring.strength * 6.f * ring.life;
      float previousX = cx + ring.radius;
      float previousY = cy;
      constexpr int kSteps = 80;
      for (int step = 1; step <= kSteps; ++step) {
        const float angle = static_cast<float>(step) / static_cast<float>(kSteps) * kPi * 2.f;
        const float wobble = 1.f + 0.03f * std::sin(angle * 6.f + ctx.time * 2.f) * ring.strength;
        const float x = cx + std::cos(angle) * ring.radius * wobble;
        const float y = cy + std::sin(angle) * ring.radius * wobble;
        ctx.batch->line(previousX, previousY, x, y, thickness, rgba(color, ring.life * (0.25f + ring.strength * 0.7f)));
        previousX = x;
        previousY = y;
      }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    ctx.batch->draw(*ctx.basic, width, height);
    glDisable(GL_BLEND);
    ctx.batch->clear();
  }

 private:
  std::vector<WaveRing> rings_;
  float cooldown_ = 0.f;
};

}  // namespace

std::unique_ptr<Mode> createRipples() {
  return std::make_unique<RipplesMode>();
}
