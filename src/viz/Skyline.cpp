#include "viz/Mode.h"

#include "gl/GlExt.h"
#include "gl/Shader.h"
#include "viz/DrawUtil.h"

#include <algorithm>

namespace {

class SkylineMode final : public Mode {
 public:
  const char* name() const override { return "Skyline"; }

  void draw(const VizContext& ctx) override {
    if (ctx.width < 2 || ctx.audio == nullptr || ctx.batch == nullptr || ctx.basic == nullptr) {
      return;
    }
    const float width = static_cast<float>(ctx.width);
    const float height = static_cast<float>(ctx.height);
    constexpr int kBands = AnalysisSnapshot::kBandCount;
    const float ground = height * 0.08f;
    const float slot = width / static_cast<float>(kBands);
    const float buildingWidth = std::max(2.f, slot * 0.78f);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    ctx.batch->clear();
    queueBackground(*ctx.batch, ctx.width, ctx.height, ctx.palette);
    ctx.batch->draw(*ctx.basic, width, height);
    ctx.batch->clear();

    ctx.batch->rect(0.f, 0.f, width, ground, rgba(paletteColor(ctx.palette, 0.08f), 0.9f));
    for (int band = 0; band < kBands; ++band) {
      const float t = static_cast<float>(band) / static_cast<float>(kBands - 1);
      const float amp = std::clamp(ctx.audio->bands[band], 0.f, 1.f);
      const float peak = std::clamp(ctx.audio->peaks[band], 0.f, 1.f);
      const float x = static_cast<float>(band) * slot + (slot - buildingWidth) * 0.5f;
      const float buildingHeight = std::max(6.f, amp * height * 0.78f);
      const Rgb color = paletteColor(ctx.palette, t);
      ctx.batch->rectVertical(x, ground, buildingWidth, buildingHeight, rgba(scaleRgb(color, 0.22f), 0.85f), rgba(scaleRgb(color, 0.55f + amp * 0.4f), 0.95f));
      ctx.batch->rect(x, ground + buildingHeight - 2.f, buildingWidth, 2.f, rgba(scaleRgb(color, 1.1f), 0.5f + peak * 0.5f));

      const float window = std::max(1.5f, buildingWidth * 0.22f);
      const float stepY = window * 2.4f;
      const int rows = std::min(12, static_cast<int>((buildingHeight - 8.f) / stepY));
      for (int row = 0; row < rows; ++row) {
        const float lit = ((band * 3 + row * 5) % 7 == 0) ? peak : amp * 0.45f;
        if (lit < 0.08f) {
          continue;
        }
        const float y = ground + 6.f + static_cast<float>(row) * stepY;
        ctx.batch->rect(x + buildingWidth * 0.18f, y, window, window, rgba(scaleRgb(color, 0.7f + lit), 0.25f + lit * 0.7f));
        ctx.batch->rect(x + buildingWidth * 0.58f, y, window, window, rgba(scaleRgb(paletteColor(ctx.palette, std::min(1.f, t + 0.15f)), 0.7f + lit), 0.2f + lit * 0.65f));
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

std::unique_ptr<Mode> createSkyline() {
  return std::make_unique<SkylineMode>();
}
