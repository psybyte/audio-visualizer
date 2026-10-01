#version 330 core
in float vHeight;
in vec2 vUv;
uniform int uPalette;
out vec4 fragColor;

#include "palette.glsl"

void main() {
  float h = clamp(vHeight, 0.0, 1.0);
  vec3 col = paletteColor(uPalette, clamp(0.08 + h * 0.92, 0.0, 1.0));
  float fx = fract(vUv.x * 48.0);
  float fz = fract(vUv.y * 36.0);
  float lx = min(fx, 1.0 - fx);
  float lz = min(fz, 1.0 - fz);
  float grid = 1.0 - smoothstep(0.0, 0.035, min(lx, lz));
  vec3 rgb = min(col * (0.28 + h * 0.85), vec3(1.0)) + vec3(0.8, 0.9, 1.0) * grid * 0.14;
  fragColor = vec4(rgb, 1.0);
}
