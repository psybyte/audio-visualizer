#pragma once

#include <cstddef>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2 0x84C2
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_RGBA16F
#define GL_RGBA16F 0x881A
#endif
#ifndef GL_R32F
#define GL_R32F 0x822E
#endif
#ifndef GL_RG
#define GL_RG 0x8227
#endif
#ifndef GL_RG32F
#define GL_RG32F 0x8230
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_PROGRAM_POINT_SIZE
#define GL_PROGRAM_POINT_SIZE 0x8642
#endif

using GLsizeiptr = ptrdiff_t;
using GLchar = char;

extern void(APIENTRY* glGenVertexArrays)(GLsizei, GLuint*);
extern void(APIENTRY* glBindVertexArray)(GLuint);
extern void(APIENTRY* glDeleteVertexArrays)(GLsizei, const GLuint*);
extern void(APIENTRY* glGenBuffers)(GLsizei, GLuint*);
extern void(APIENTRY* glBindBuffer)(GLenum, GLuint);
extern void(APIENTRY* glBufferData)(GLenum, GLsizeiptr, const void*, GLenum);
extern void(APIENTRY* glDeleteBuffers)(GLsizei, const GLuint*);
extern void(APIENTRY* glEnableVertexAttribArray)(GLuint);
extern void(APIENTRY* glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
extern GLuint(APIENTRY* glCreateShader)(GLenum);
extern void(APIENTRY* glShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*);
extern void(APIENTRY* glCompileShader)(GLuint);
extern void(APIENTRY* glGetShaderiv)(GLuint, GLenum, GLint*);
extern void(APIENTRY* glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
extern void(APIENTRY* glDeleteShader)(GLuint);
extern GLuint(APIENTRY* glCreateProgram)(void);
extern void(APIENTRY* glAttachShader)(GLuint, GLuint);
extern void(APIENTRY* glLinkProgram)(GLuint);
extern void(APIENTRY* glGetProgramiv)(GLuint, GLenum, GLint*);
extern void(APIENTRY* glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
extern void(APIENTRY* glUseProgram)(GLuint);
extern void(APIENTRY* glDeleteProgram)(GLuint);
extern GLint(APIENTRY* glGetUniformLocation)(GLuint, const GLchar*);
extern void(APIENTRY* glUniform1i)(GLint, GLint);
extern void(APIENTRY* glUniform1f)(GLint, GLfloat);
extern void(APIENTRY* glUniform2f)(GLint, GLfloat, GLfloat);
extern void(APIENTRY* glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);
extern void(APIENTRY* glActiveTexture)(GLenum);
extern void(APIENTRY* glGenFramebuffers)(GLsizei, GLuint*);
extern void(APIENTRY* glBindFramebuffer)(GLenum, GLuint);
extern void(APIENTRY* glFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
extern GLenum(APIENTRY* glCheckFramebufferStatus)(GLenum);
extern void(APIENTRY* glDeleteFramebuffers)(GLsizei, const GLuint*);

void initGlExtensions();

GLuint makeR32fTexture(int width, int height);
GLuint makeRg32fTexture(int width, int height);
GLuint makeColorTexture(int width, int height, bool halfFloat);
void destroyTexture(GLuint texture);
