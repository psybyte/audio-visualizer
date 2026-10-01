#pragma once

#include "gl/ScreenQuad.h"
#include "gl/Shader.h"

class Trail {
 public:
  Trail(const Shader& fade, const Shader& blit);
  ~Trail();
  Trail(const Trail&) = delete;
  Trail& operator=(const Trail&) = delete;

  void clear();
  void begin(int width, int height, float fade);
  void end(int width, int height);

 private:
  void resize(int width, int height);

  const Shader* fade_ = nullptr;
  const Shader* blit_ = nullptr;
  ScreenQuad quad_;
  unsigned int fbo_[2] = {};
  unsigned int tex_[2] = {};
  int width_ = 0;
  int height_ = 0;
  int src_ = 0;
  int dst_ = 1;
};
