#include "viz/Mode.h"

#include "gl/GlExt.h"
#include "gl/ScreenQuad.h"
#include "gl/Shader.h"

#include <vector>

namespace {

class SpectrogramMode final : public Mode {
 public:
  SpectrogramMode() {
    texture_ = makeR32fTexture(kHistory, AnalysisSnapshot::kBinCount);
  }

  ~SpectrogramMode() override { destroyTexture(texture_); }

  const char* name() const override { return kModeNames[3]; }

  void reset() override {
    std::vector<float> zeros(static_cast<size_t>(kHistory) * AnalysisSnapshot::kBinCount, 0.f);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kHistory, AnalysisSnapshot::kBinCount, GL_RED, GL_FLOAT, zeros.data());
    column_ = 0;
  }

  void draw(const VizContext& ctx) override {
    if (ctx.audio == nullptr || ctx.spectrogram == nullptr || ctx.quad == nullptr) {
      return;
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, column_, 0, 1, AnalysisSnapshot::kBinCount, GL_RED, GL_FLOAT, ctx.audio->bins);
    ctx.spectrogram->use();
    ctx.spectrogram->set1i("uHist", 0);
    ctx.spectrogram->set1i("uPalette", static_cast<int>(ctx.palette));
    ctx.spectrogram->set1f("uNewest", static_cast<float>(column_));
    ctx.spectrogram->set1f("uWidth", static_cast<float>(kHistory));
    ctx.spectrogram->set1f("uBins", static_cast<float>(AnalysisSnapshot::kBinCount));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    ctx.quad->draw();
    column_ = (column_ + 1) % kHistory;
  }

 private:
  static constexpr int kHistory = 640;
  unsigned int texture_ = 0;
  int column_ = 0;
};

}  // namespace

std::unique_ptr<Mode> createSpectrogram() {
  return std::make_unique<SpectrogramMode>();
}
