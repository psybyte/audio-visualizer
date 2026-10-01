#pragma once

class ScreenQuad {
 public:
  ScreenQuad();
  ~ScreenQuad();
  ScreenQuad(const ScreenQuad&) = delete;
  ScreenQuad& operator=(const ScreenQuad&) = delete;

  void draw() const;

 private:
  unsigned int vao_ = 0;
  unsigned int vbo_ = 0;
};
