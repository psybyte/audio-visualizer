#include "gl/Shader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

std::string preprocess(const std::filesystem::path& path, int depth) {
  if (depth > 4) {
    throw std::runtime_error("Shader include is too deep: " + path.string());
  }
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Could not open shader " + path.string());
  }
  std::ostringstream output;
  std::string line;
  while (std::getline(input, line)) {
    if (line.rfind("#include", 0) == 0) {
      const auto first = line.find('"');
      const auto second = first == std::string::npos ? std::string::npos : line.find('"', first + 1);
      if (first == std::string::npos || second == std::string::npos) {
        throw std::runtime_error("Invalid include in " + path.string());
      }
      const auto includeName = line.substr(first + 1, second - first - 1);
      output << preprocess(path.parent_path() / includeName, depth + 1) << '\n';
    } else {
      output << line << '\n';
    }
  }
  return output.str();
}

GLuint compileStage(GLenum type, const std::string& source, const std::string& label) {
  const GLuint shader = glCreateShader(type);
  const char* text = source.c_str();
  glShaderSource(shader, 1, &text, nullptr);
  glCompileShader(shader);
  GLint ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<size_t>(length > 1 ? length : 1), '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    glDeleteShader(shader);
    throw std::runtime_error("Failed to compile " + label + ":\n" + log);
  }
  return shader;
}

}  // namespace

Shader::Shader(Shader&& other) noexcept : program_(other.program_) {
  other.program_ = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
  if (this != &other) {
    if (program_ != 0) {
      glDeleteProgram(program_);
    }
    program_ = other.program_;
    other.program_ = 0;
  }
  return *this;
}

Shader::~Shader() {
  if (program_ != 0) {
    glDeleteProgram(program_);
  }
}

Shader Shader::fromSources(const std::string& vertexSource, const std::string& fragmentSource, const std::string& label) {
  const GLuint vs = compileStage(GL_VERTEX_SHADER, vertexSource, label);
  const GLuint fs = compileStage(GL_FRAGMENT_SHADER, fragmentSource, label);
  const GLuint program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glDeleteShader(vs);
  glDeleteShader(fs);
  GLint ok = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (!ok) {
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<size_t>(length > 1 ? length : 1), '\0');
    glGetProgramInfoLog(program, length, nullptr, log.data());
    glDeleteProgram(program);
    throw std::runtime_error("Failed to link " + label + ":\n" + log);
  }
  return Shader(program);
}

Shader Shader::fromFiles(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) {
  return fromSources(preprocess(vertexPath, 0), preprocess(fragmentPath, 0), vertexPath.filename().string());
}

void Shader::use() const {
  glUseProgram(program_);
}

GLint Shader::location(const char* name) const {
  return glGetUniformLocation(program_, name);
}

void Shader::set1i(const char* name, int value) const {
  glUniform1i(location(name), value);
}

void Shader::set1f(const char* name, float value) const {
  glUniform1f(location(name), value);
}

void Shader::set2f(const char* name, float x, float y) const {
  glUniform2f(location(name), x, y);
}

void Shader::setMat4(const char* name, const float* columnMajor) const {
  glUniformMatrix4fv(location(name), 1, GL_FALSE, columnMajor);
}
