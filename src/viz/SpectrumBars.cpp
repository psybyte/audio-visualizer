#include "viz/Mode.h"

#include "gl/GlExt.h"
#include "gl/Shader.h"
#include "viz/DrawUtil.h"

#include <algorithm>

namespace {

class SpectrumBars final : public Mode {
 public:
  const char* name() const override { return kModeNames[0]; }

  void draw(const VizContext& ctx) override {
    if (ctx.width < 2 || ctx.height < 2 || ctx.audio == nullptr || ctx.batch == nullptr || ctx.basic == nullptr) {
      return;
    }
    const float width = static_cast<float>(ctx.width);
    const float height = static_cast<float>(ctx.height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    ctx.batch->clear();
    queueBackground(*ctx.batch, ctx.width, ctx.height, ctx.palette);
    ctx.batch->draw(*ctx.basic, width, height);

    ctx.batch->clear();
    constexpr int kBands = AnalysisSnapshot::kBandCount;
    const float margin = width * 0.045f;
    const float slot = (width - margin * 2.f) / static_cast<float>(kBands);
    const float barWidth = std::max(1.f, slot * 0.72f);
    const float base = height * 0.16f;
    const float maxHeight = height * 0.74f;
    ctx.batch->line(margin, base, width - margin, base, 1.5f, rgba(paletteColor(ctx.palette, 0.55f), 0.35f));
    for (int i = 0; i < kBands; ++i) {
      const float t = static_cast<float>(i) / static_cast<float>(kBands - 1);
      const float amp = std::clamp(ctx.audio->bands[i], 0.f, 1.f);
      const float peak = std::clamp(ctx.audio->peaks[i], 0.f, 1.f);
      const float x = margin + static_cast<float>(i) * slot + (slot - barWidth) * 0.5f;
      const float barHeight = std::max(2.f, amp * maxHeight);
      const Rgb color = paletteColor(ctx.palette, t);
      const Rgba glow = rgba(color, 0.08f + amp * 0.20f);
      const Rgba core = rgba(scaleRgb(color, 0.30f + amp * 0.9f), 0.50f + amp * 0.5f);
      const Rgba cap = rgba(scaleRgb(paletteColor(ctx.palette, std::min(1.f, t + 0.15f)), 1.15f), 0.9f);
      ctx.batch->rect(x - barWidth * 0.28f, base, barWidth * 1.56f, barHeight, glow);
      ctx.batch->rect(x, base - barHeight * 0.34f, barWidth, barHeight * 0.34f, rgba(color, 0.05f + amp * 0.16f));
      ctx.batch->rect(x, base, barWidth, barHeight, core);
      ctx.batch->line(x, base + peak * maxHeight, x + barWidth, base + peak * maxHeight, 2.f, cap);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    ctx.batch->draw(*ctx.basic, width, height);
    glDisable(GL_BLEND);
    ctx.batch->clear();
  }
};

}  // namespace

std::unique_ptr<Mode> createSpectrumBars() {
  return std::make_unique<SpectrumBars>();
}
