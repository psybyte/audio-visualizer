#pragma once

#include "viz/Palette.h"

#include <vector>

class Shader;

class Batch {
 public:
  Batch();
  ~Batch();
  Batch(const Batch&) = delete;
  Batch& operator=(const Batch&) = delete;

  void clear();
  void rect(float x, float y, float width, float height, Rgba color);
  void rectVertical(float x, float y, float width, float height, Rgba bottom, Rgba top);
  void line(float x0, float y0, float x1, float y1, float thickness, Rgba color);
  void disc(float cx, float cy, float radius, Rgba color, int segments = 40);
  void draw(const Shader& shader, float screenWidth, float screenHeight);

 private:
  void vertex(float x, float y, Rgba color);

  std::vector<float> vertices_;
  unsigned int vao_ = 0;
  unsigned int vbo_ = 0;
};
