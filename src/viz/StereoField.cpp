#include "viz/Mode.h"

#include "gl/GlExt.h"
#include "gl/Shader.h"
#include "viz/DrawUtil.h"

#include <algorithm>

namespace {

class StereoFieldMode final : public Mode {
 public:
  const char* name() const override { return "Stereo"; }

  void draw(const VizContext& ctx) override {
    if (ctx.width < 2 || ctx.audio == nullptr || ctx.batch == nullptr || ctx.basic == nullptr) {
      return;
    }
    const float width = static_cast<float>(ctx.width);
    const float height = static_cast<float>(ctx.height);
    const float midY = height * 0.5f;
    const float amp = height * 0.34f;
    constexpr int kCount = AnalysisSnapshot::kWaveCount;
    constexpr int kStep = 4;

    float correlation = 0.f;
    int samples = 0;
    for (int i = 0; i < kCount; i += kStep) {
      correlation += ctx.audio->waveL[i] * ctx.audio->waveR[i];
      ++samples;
    }
    correlation = samples > 0 ? std::clamp(correlation / static_cast<float>(samples), -1.f, 1.f) : 0.f;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    ctx.batch->clear();
    queueBackground(*ctx.batch, ctx.width, ctx.height, ctx.palette);
    ctx.batch->draw(*ctx.basic, width, height);
    ctx.batch->clear();

    ctx.batch->line(width * 0.5f, height * 0.12f, width * 0.5f, height * 0.88f, 1.4f, rgba(paletteColor(ctx.palette, 0.5f), 0.35f));
    const Rgb leftColor = paletteColor(ctx.palette, 0.18f);
    const Rgb rightColor = paletteColor(ctx.palette, 0.82f);
    for (int i = 0; i + kStep < kCount; i += kStep) {
      const float x0 = static_cast<float>(i) / static_cast<float>(kCount - 1) * width;
      const float yL = midY + ctx.audio->waveL[i] * amp;
      const float yR = midY + ctx.audio->waveR[i] * amp;
      const float thickness = std::max(2.f, width / static_cast<float>(kCount / kStep) * 0.9f);
      if (x0 < width * 0.5f) {
        ctx.batch->line(x0, midY, x0, yL, thickness, rgba(leftColor, 0.55f));
        ctx.batch->line(x0, midY, x0, midY - (yL - midY) * 0.35f, thickness, rgba(leftColor, 0.18f));
      } else {
        ctx.batch->line(x0, midY, x0, yR, thickness, rgba(rightColor, 0.55f));
        ctx.batch->line(x0, midY, x0, midY - (yR - midY) * 0.35f, thickness, rgba(rightColor, 0.18f));
      }
    }

    const float marker = width * 0.5f + correlation * width * 0.28f;
    ctx.batch->disc(marker, height * 0.1f, 7.f + std::fabs(correlation) * 8.f, rgba(paletteColor(ctx.palette, 0.5f + correlation * 0.4f), 0.8f), 16);
    ctx.batch->line(width * 0.22f, height * 0.1f, width * 0.78f, height * 0.1f, 1.5f, rgba(paletteColor(ctx.palette, 0.6f), 0.35f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    ctx.batch->draw(*ctx.basic, width, height);
    glDisable(GL_BLEND);
    ctx.batch->clear();
  }
};

}  // namespace

std::unique_ptr<Mode> createStereoField() {
  return std::make_unique<StereoFieldMode>();
}
