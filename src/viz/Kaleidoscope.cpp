#include "viz/Mode.h"

#include "gl/Batch.h"
#include "gl/GlExt.h"
#include "gl/ScreenQuad.h"
#include "gl/Shader.h"
#include "viz/RadialDraw.h"

#include <stdexcept>

namespace {

class KaleidoscopeMode final : public Mode {
 public:
  KaleidoscopeMode() {
    texture_ = makeColorTexture(kSize, kSize, false);
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      throw std::runtime_error("Could not create the kaleidoscope.");
    }
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glViewport(0, 0, kSize, kSize);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  ~KaleidoscopeMode() override {
    if (fbo_ != 0) {
      glDeleteFramebuffers(1, &fbo_);
    }
    destroyTexture(texture_);
  }

  const char* name() const override { return kModeNames[7]; }

  void reset() override {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, kSize, kSize);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void draw(const VizContext& ctx) override {
    if (ctx.audio == nullptr || ctx.batch == nullptr || ctx.basic == nullptr || ctx.kaleidoscope == nullptr || ctx.quad == nullptr) {
      return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, kSize, kSize);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    ctx.batch->clear();
    drawRadial(*ctx.batch, *ctx.audio, ctx.palette, ctx.time * 0.65f, kSize, kSize);
    ctx.batch->draw(*ctx.basic, static_cast<float>(kSize), static_cast<float>(kSize));
    ctx.batch->clear();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ctx.width, ctx.height);
    glDisable(GL_BLEND);
    ctx.kaleidoscope->use();
    ctx.kaleidoscope->set1i("uTex", 0);
    ctx.kaleidoscope->set1f("uTime", ctx.time);
    ctx.kaleidoscope->set1f("uSegments", 8.f);
    ctx.kaleidoscope->set1f("uSpin", 0.18f + ctx.audio->treble * 0.45f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    ctx.quad->draw();
  }

 private:
  static constexpr int kSize = 1024;
  unsigned int texture_ = 0;
  unsigned int fbo_ = 0;
};

}  // namespace

std::unique_ptr<Mode> createKaleidoscope() {
  return std::make_unique<KaleidoscopeMode>();
}
