#include "gl/ScreenQuad.h"

#include "gl/GlExt.h"

ScreenQuad::ScreenQuad() {
  const float vertices[] = {
      -1.f, -1.f, 0.f, 0.f,
      1.f,  -1.f, 1.f, 0.f,
      1.f,  1.f,  1.f, 1.f,
      -1.f, -1.f, 0.f, 0.f,
      1.f,  1.f,  1.f, 1.f,
      -1.f, 1.f,  0.f, 1.f,
  };
  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);
  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
  glBindVertexArray(0);
}

ScreenQuad::~ScreenQuad() {
  if (vbo_ != 0) {
    glDeleteBuffers(1, &vbo_);
  }
  if (vao_ != 0) {
    glDeleteVertexArrays(1, &vao_);
  }
}

void ScreenQuad::draw() const {
  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}
