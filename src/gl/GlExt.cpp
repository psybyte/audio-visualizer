#include "gl/GlExt.h"

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

void(APIENTRY* glGenVertexArrays)(GLsizei, GLuint*) = nullptr;
void(APIENTRY* glBindVertexArray)(GLuint) = nullptr;
void(APIENTRY* glDeleteVertexArrays)(GLsizei, const GLuint*) = nullptr;
void(APIENTRY* glGenBuffers)(GLsizei, GLuint*) = nullptr;
void(APIENTRY* glBindBuffer)(GLenum, GLuint) = nullptr;
void(APIENTRY* glBufferData)(GLenum, GLsizeiptr, const void*, GLenum) = nullptr;
void(APIENTRY* glDeleteBuffers)(GLsizei, const GLuint*) = nullptr;
void(APIENTRY* glEnableVertexAttribArray)(GLuint) = nullptr;
void(APIENTRY* glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) = nullptr;
GLuint(APIENTRY* glCreateShader)(GLenum) = nullptr;
void(APIENTRY* glShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*) = nullptr;
void(APIENTRY* glCompileShader)(GLuint) = nullptr;
void(APIENTRY* glGetShaderiv)(GLuint, GLenum, GLint*) = nullptr;
void(APIENTRY* glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
void(APIENTRY* glDeleteShader)(GLuint) = nullptr;
GLuint(APIENTRY* glCreateProgram)(void) = nullptr;
void(APIENTRY* glAttachShader)(GLuint, GLuint) = nullptr;
void(APIENTRY* glLinkProgram)(GLuint) = nullptr;
void(APIENTRY* glGetProgramiv)(GLuint, GLenum, GLint*) = nullptr;
void(APIENTRY* glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
void(APIENTRY* glUseProgram)(GLuint) = nullptr;
void(APIENTRY* glDeleteProgram)(GLuint) = nullptr;
GLint(APIENTRY* glGetUniformLocation)(GLuint, const GLchar*) = nullptr;
void(APIENTRY* glUniform1i)(GLint, GLint) = nullptr;
void(APIENTRY* glUniform1f)(GLint, GLfloat) = nullptr;
void(APIENTRY* glUniform2f)(GLint, GLfloat, GLfloat) = nullptr;
void(APIENTRY* glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
void(APIENTRY* glActiveTexture)(GLenum) = nullptr;
void(APIENTRY* glGenFramebuffers)(GLsizei, GLuint*) = nullptr;
void(APIENTRY* glBindFramebuffer)(GLenum, GLuint) = nullptr;
void(APIENTRY* glFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint) = nullptr;
GLenum(APIENTRY* glCheckFramebufferStatus)(GLenum) = nullptr;
void(APIENTRY* glDeleteFramebuffers)(GLsizei, const GLuint*) = nullptr;

namespace {

template <typename T>
void loadProc(T& fn, const char* name, std::string& missing) {
  fn = reinterpret_cast<T>(glfwGetProcAddress(name));
  if (!fn) {
    if (!missing.empty()) {
      missing += ", ";
    }
    missing += name;
  }
}

void applySampler(GLuint texture) {
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

}  // namespace

void initGlExtensions() {
  std::string missing;
  loadProc(glGenVertexArrays, "glGenVertexArrays", missing);
  loadProc(glBindVertexArray, "glBindVertexArray", missing);
  loadProc(glDeleteVertexArrays, "glDeleteVertexArrays", missing);
  loadProc(glGenBuffers, "glGenBuffers", missing);
  loadProc(glBindBuffer, "glBindBuffer", missing);
  loadProc(glBufferData, "glBufferData", missing);
  loadProc(glDeleteBuffers, "glDeleteBuffers", missing);
  loadProc(glEnableVertexAttribArray, "glEnableVertexAttribArray", missing);
  loadProc(glVertexAttribPointer, "glVertexAttribPointer", missing);
  loadProc(glCreateShader, "glCreateShader", missing);
  loadProc(glShaderSource, "glShaderSource", missing);
  loadProc(glCompileShader, "glCompileShader", missing);
  loadProc(glGetShaderiv, "glGetShaderiv", missing);
  loadProc(glGetShaderInfoLog, "glGetShaderInfoLog", missing);
  loadProc(glDeleteShader, "glDeleteShader", missing);
  loadProc(glCreateProgram, "glCreateProgram", missing);
  loadProc(glAttachShader, "glAttachShader", missing);
  loadProc(glLinkProgram, "glLinkProgram", missing);
  loadProc(glGetProgramiv, "glGetProgramiv", missing);
  loadProc(glGetProgramInfoLog, "glGetProgramInfoLog", missing);
  loadProc(glUseProgram, "glUseProgram", missing);
  loadProc(glDeleteProgram, "glDeleteProgram", missing);
  loadProc(glGetUniformLocation, "glGetUniformLocation", missing);
  loadProc(glUniform1i, "glUniform1i", missing);
  loadProc(glUniform1f, "glUniform1f", missing);
  loadProc(glUniform2f, "glUniform2f", missing);
  loadProc(glUniformMatrix4fv, "glUniformMatrix4fv", missing);
  loadProc(glActiveTexture, "glActiveTexture", missing);
  loadProc(glGenFramebuffers, "glGenFramebuffers", missing);
  loadProc(glBindFramebuffer, "glBindFramebuffer", missing);
  loadProc(glFramebufferTexture2D, "glFramebufferTexture2D", missing);
  loadProc(glCheckFramebufferStatus, "glCheckFramebufferStatus", missing);
  loadProc(glDeleteFramebuffers, "glDeleteFramebuffers", missing);
  if (!missing.empty()) {
    throw std::runtime_error("Incomplete OpenGL: " + missing);
  }
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
}

GLuint makeR32fTexture(int width, int height) {
  GLuint texture = 0;
  glGenTextures(1, &texture);
  applySampler(texture);
  std::vector<float> zeros(static_cast<size_t>(width) * static_cast<size_t>(height), 0.f);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, zeros.data());
  return texture;
}

GLuint makeColorTexture(int width, int height, bool halfFloat) {
  GLuint texture = 0;
  glGenTextures(1, &texture);
  applySampler(texture);
  const GLenum internal = halfFloat ? GL_RGBA16F : GL_RGBA8;
  glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
  return texture;
}

void destroyTexture(GLuint texture) {
  if (texture != 0) {
    glDeleteTextures(1, &texture);
  }
}
