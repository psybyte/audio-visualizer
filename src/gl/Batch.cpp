#include "gl/Batch.h"

#include "gl/GlExt.h"
#include "gl/Shader.h"

#include <cmath>

Batch::Batch() {
  vertices_.reserve(48000);
  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);
  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
  glBindVertexArray(0);
}

Batch::~Batch() {
  if (vbo_ != 0) {
    glDeleteBuffers(1, &vbo_);
  }
  if (vao_ != 0) {
    glDeleteVertexArrays(1, &vao_);
  }
}

void Batch::clear() {
  vertices_.clear();
}

void Batch::vertex(float x, float y, Rgba color) {
  vertices_.push_back(x);
  vertices_.push_back(y);
  vertices_.push_back(color.r);
  vertices_.push_back(color.g);
  vertices_.push_back(color.b);
  vertices_.push_back(color.a);
}

void Batch::rect(float x, float y, float width, float height, Rgba color) {
  rectVertical(x, y, width, height, color, color);
}

void Batch::rectVertical(float x, float y, float width, float height, Rgba bottom, Rgba top) {
  if (width <= 0.f || height == 0.f) {
    return;
  }
  vertex(x, y, bottom);
  vertex(x + width, y, bottom);
  vertex(x + width, y + height, top);
  vertex(x, y, bottom);
  vertex(x + width, y + height, top);
  vertex(x, y + height, top);
}

void Batch::line(float x0, float y0, float x1, float y1, float thickness, Rgba color) {
  const float dx = x1 - x0;
  const float dy = y1 - y0;
  const float len = std::sqrt(dx * dx + dy * dy);
  if (len < 0.001f || thickness <= 0.f) {
    return;
  }
  const float nx = -dy / len * (thickness * 0.5f);
  const float ny = dx / len * (thickness * 0.5f);
  vertex(x0 + nx, y0 + ny, color);
  vertex(x0 - nx, y0 - ny, color);
  vertex(x1 - nx, y1 - ny, color);
  vertex(x0 + nx, y0 + ny, color);
  vertex(x1 - nx, y1 - ny, color);
  vertex(x1 + nx, y1 + ny, color);
}

void Batch::disc(float cx, float cy, float radius, Rgba color, int segments) {
  if (radius <= 0.f || segments < 3) {
    return;
  }
  constexpr float kPi = 3.14159265f;
  for (int i = 0; i < segments; ++i) {
    const float a0 = static_cast<float>(i) / static_cast<float>(segments) * kPi * 2.f;
    const float a1 = static_cast<float>(i + 1) / static_cast<float>(segments) * kPi * 2.f;
    vertex(cx, cy, color);
    vertex(cx + std::cos(a0) * radius, cy + std::sin(a0) * radius, color);
    vertex(cx + std::cos(a1) * radius, cy + std::sin(a1) * radius, color);
  }
}

void Batch::draw(const Shader& shader, float screenWidth, float screenHeight) {
  if (vertices_.empty() || screenWidth < 1.f || screenHeight < 1.f) {
    return;
  }
  shader.use();
  shader.set2f("uResolution", screenWidth, screenHeight);
  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)), vertices_.data(), GL_DYNAMIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size() / 6));
  glBindVertexArray(0);
}
