#version 330 core
layout(location = 0) in vec2 aUv;
uniform sampler2D uHeight;
uniform mat4 uMvp;
uniform float uNewest;
uniform float uRows;
uniform float uScale;
out float vHeight;
out vec2 vUv;
void main() {
  float rowIndex = uNewest - (1.0 - aUv.y) * (uRows - 1.0);
  float texY = (mod(rowIndex, uRows) + 0.5) / uRows;
  float h = texture(uHeight, vec2(aUv.x, texY)).r;
  vHeight = h;
  vUv = aUv;
  vec3 pos = vec3((aUv.x - 0.5) * 4.4, h * uScale, (aUv.y - 0.5) * 3.6);
  gl_Position = uMvp * vec4(pos, 1.0);
}
