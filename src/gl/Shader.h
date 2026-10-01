#pragma once

#include "gl/GlExt.h"

#include <filesystem>
#include <string>

class Shader {
 public:
  Shader() = default;
  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;
  Shader(Shader&& other) noexcept;
  Shader& operator=(Shader&& other) noexcept;
  ~Shader();

  static Shader fromFiles(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
  static Shader fromSources(const std::string& vertexSource, const std::string& fragmentSource, const std::string& label);

  void use() const;
  void set1i(const char* name, int value) const;
  void set1f(const char* name, float value) const;
  void set2f(const char* name, float x, float y) const;
  void setMat4(const char* name, const float* columnMajor) const;
  GLuint id() const { return program_; }

 private:
  explicit Shader(GLuint program) : program_(program) {}
  GLint location(const char* name) const;

  GLuint program_ = 0;
};
