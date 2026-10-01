#include "viz/Trail.h"

#include "gl/GlExt.h"

#include <stdexcept>

Trail::Trail(const Shader& fade, const Shader& blit) : fade_(&fade), blit_(&blit) {}

Trail::~Trail() {
  if (fbo_[0] != 0 || fbo_[1] != 0) {
    glDeleteFramebuffers(2, fbo_);
  }
  destroyTexture(tex_[0]);
  destroyTexture(tex_[1]);
}

void Trail::resize(int width, int height) {
  if (width == width_ && height == height_ && fbo_[0] != 0) {
    return;
  }
  if (fbo_[0] != 0) {
    glDeleteFramebuffers(2, fbo_);
    fbo_[0] = fbo_[1] = 0;
  }
  destroyTexture(tex_[0]);
  destroyTexture(tex_[1]);
  tex_[0] = tex_[1] = 0;
  width_ = width;
  height_ = height;
  src_ = 0;
  dst_ = 1;
  glGenFramebuffers(2, fbo_);
  for (int i = 0; i < 2; ++i) {
    tex_[i] = makeColorTexture(width, height, true);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_[i], 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      throw std::runtime_error("Could not create the oscilloscope trail.");
    }
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Trail::clear() {
  if (width_ < 2 || fbo_[0] == 0) {
    return;
  }
  glDisable(GL_BLEND);
  glClearColor(0.f, 0.f, 0.f, 1.f);
  for (int i = 0; i < 2; ++i) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_[i]);
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Trail::begin(int width, int height, float fade) {
  if (width < 2 || height < 2) {
    return;
  }
  resize(width, height);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_[dst_]);
  glViewport(0, 0, width_, height_);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  fade_->use();
  fade_->set1i("uTex", 0);
  fade_->set1f("uFade", fade);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex_[src_]);
  quad_.draw();
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void Trail::end(int width, int height) {
  if (width_ < 2 || fbo_[0] == 0) {
    return;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, width, height);
  glDisable(GL_BLEND);
  blit_->use();
  blit_->set1i("uTex", 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex_[dst_]);
  quad_.draw();
  const int previous = src_;
  src_ = dst_;
  dst_ = previous;
}
