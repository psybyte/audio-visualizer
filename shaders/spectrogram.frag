#version 330 core
in vec2 vUv;
uniform sampler2D uHist;
uniform int uPalette;
uniform float uNewest;
uniform float uWidth;
uniform float uBins;
out vec4 fragColor;

#include "palette.glsl"

void main() {
  float columnsBack = (1.0 - vUv.x) * (uWidth - 1.0);
  float index = uNewest - columnsBack;
  float texX = (mod(index, uWidth) + 0.5) / uWidth;
  float bin = exp(mix(log(1.0), log(max(uBins - 1.0, 2.0)), clamp(vUv.y, 0.0, 1.0)));
  float texY = (bin + 0.5) / uBins;
  float v = texture(uHist, vec2(texX, texY)).r;
  float glow = texture(uHist, vec2(texX, clamp(texY + 2.0 / uBins, 0.0, 1.0))).r;
  float mag = clamp(max(v, glow * 0.55), 0.0, 1.0);
  float presence = smoothstep(0.025, 0.16, mag);
  vec3 col = paletteColor(uPalette, clamp(0.12 + mag * 0.88, 0.0, 1.0));
  vec3 rgb = col * presence * (0.20 + mag * 1.25);
  float shade = smoothstep(0.0, 0.06, vUv.y) * smoothstep(1.0, 0.94, vUv.y);
  fragColor = vec4(rgb * shade, 1.0);
}
