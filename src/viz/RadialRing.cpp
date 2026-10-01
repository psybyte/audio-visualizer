#include "viz/Mode.h"

#include "gl/GlExt.h"
#include "gl/Shader.h"
#include "viz/DrawUtil.h"
#include "viz/Palette.h"
#include "viz/RadialDraw.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265f;

class RadialRing final : public Mode {
 public:
  const char* name() const override { return kModeNames[1]; }

  void draw(const VizContext& ctx) override {
    if (ctx.width < 2 || ctx.audio == nullptr || ctx.batch == nullptr || ctx.basic == nullptr) {
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
    drawRadial(*ctx.batch, *ctx.audio, ctx.palette, ctx.time, ctx.width, ctx.height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    ctx.batch->draw(*ctx.basic, width, height);
    glDisable(GL_BLEND);
    ctx.batch->clear();
  }
};

}  // namespace

void drawRadial(Batch& batch, const AnalysisSnapshot& audio, Palette palette, float time, int width, int height) {
  const float cx = static_cast<float>(width) * 0.5f;
  const float cy = static_cast<float>(height) * 0.5f;
  const float minSide = static_cast<float>(std::min(width, height));
  const float inner = minSide * 0.18f;
  const float maxLength = minSide * 0.30f;
  constexpr int kBands = AnalysisSnapshot::kBandCount;
  const float circumference = 2.f * kPi * (inner + maxLength * 0.45f);
  const float thickness = std::clamp(circumference / static_cast<float>(kBands) * 0.62f, 2.f, 14.f);
  const float rotation = time * 0.12f;

  const Rgb coreColor = paletteColor(palette, 0.2f + audio.bass * 0.5f);
  batch.disc(cx, cy, inner * (0.22f + audio.bass * 0.18f), rgba(coreColor, 0.28f + audio.bass * 0.35f), 36);

  for (int i = 0; i < kBands; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(kBands);
    const float angle = -kPi * 0.5f + (static_cast<float>(i) + 0.5f) / static_cast<float>(kBands) * kPi * 2.f + rotation;
    const float amp = std::clamp(audio.bands[i], 0.f, 1.f);
    const float length = 6.f + amp * maxLength;
    const float cosine = std::cos(angle);
    const float sine = std::sin(angle);
    const Rgb color = paletteColor(palette, t);
    batch.line(cx + cosine * inner, cy + sine * inner, cx + cosine * (inner + length), cy + sine * (inner + length), thickness,
               rgba(scaleRgb(color, 0.35f + amp), 0.35f + amp * 0.65f));
  }

  constexpr int kRing = 640;
  const int stride = std::max(1, AnalysisSnapshot::kWaveCount / kRing);
  float previousX = 0.f;
  float previousY = 0.f;
  bool hasPrevious = false;
  const float ring = inner * 0.78f;
  const Rgba ringColor = rgba(paletteColor(palette, 0.75f), 0.8f);
  for (int i = 0; i <= kRing; ++i) {
    const int sample = std::min((i % kRing) * stride, AnalysisSnapshot::kWaveCount - 1);
    const float angle = static_cast<float>(i % kRing) / static_cast<float>(kRing) * kPi * 2.f + rotation * 0.5f;
    const float radius = ring + audio.waveL[sample] * ring * 0.42f;
    const float x = cx + std::cos(angle) * radius;
    const float y = cy + std::sin(angle) * radius;
    if (hasPrevious) {
      batch.line(previousX, previousY, x, y, 2.2f, ringColor);
    }
    previousX = x;
    previousY = y;
    hasPrevious = true;
  }
}

std::unique_ptr<Mode> createRadialRing() {
  return std::make_unique<RadialRing>();
}
