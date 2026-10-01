#pragma once

#include <cmath>

struct Vec3 {
  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
};

struct Mat4 {
  float m[16] = {
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1,
  };
};

inline float dot(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline Vec3 sub(Vec3 a, Vec3 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline float length(Vec3 v) {
  return std::sqrt(dot(v, v));
}

inline Vec3 normalize(Vec3 v) {
  const float len = length(v);
  if (len < 1e-6f) {
    return {0.f, 0.f, 0.f};
  }
  return {v.x / len, v.y / len, v.z / len};
}

inline Mat4 multiply(const Mat4& a, const Mat4& b) {
  Mat4 result{};
  for (int i = 0; i < 16; ++i) {
    result.m[i] = 0.f;
  }
  for (int column = 0; column < 4; ++column) {
    for (int row = 0; row < 4; ++row) {
      float sum = 0.f;
      for (int k = 0; k < 4; ++k) {
        sum += a.m[k * 4 + row] * b.m[column * 4 + k];
      }
      result.m[column * 4 + row] = sum;
    }
  }
  return result;
}

inline Mat4 perspective(float fovYRadians, float aspect, float nearPlane, float farPlane) {
  const float t = 1.f / std::tan(fovYRadians * 0.5f);
  Mat4 m{};
  for (float& value : m.m) {
    value = 0.f;
  }
  m.m[0] = t / aspect;
  m.m[5] = t;
  m.m[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
  m.m[11] = -1.f;
  m.m[14] = (2.f * farPlane * nearPlane) / (nearPlane - farPlane);
  return m;
}

inline Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
  Vec3 forward = normalize(sub(center, eye));
  Vec3 side = normalize(cross(forward, up));
  if (length(side) < 1e-4f) {
    side = normalize(cross(forward, Vec3{0.f, 0.f, 1.f}));
  }
  const Vec3 lifted = cross(side, forward);
  Mat4 m{};
  for (float& value : m.m) {
    value = 0.f;
  }
  m.m[0] = side.x;
  m.m[4] = side.y;
  m.m[8] = side.z;
  m.m[12] = -dot(side, eye);
  m.m[1] = lifted.x;
  m.m[5] = lifted.y;
  m.m[9] = lifted.z;
  m.m[13] = -dot(lifted, eye);
  m.m[2] = -forward.x;
  m.m[6] = -forward.y;
  m.m[10] = -forward.z;
  m.m[14] = dot(forward, eye);
  m.m[15] = 1.f;
  return m;
}
