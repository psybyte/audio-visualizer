#version 330 core
in vec2 vUv;
uniform sampler2D uTex;
uniform float uTime;
uniform float uSegments;
uniform float uSpin;
out vec4 fragColor;
void main() {
  vec2 p = vUv * 2.0 - 1.0;
  float r = length(p);
  float a = atan(p.y, p.x) + uTime * uSpin;
  float seg = 6.28318530718 / uSegments;
  a = mod(a, seg);
  if (a < 0.0) {
    a += seg;
  }
  a = abs(a - seg * 0.5);
  vec2 q = vec2(cos(a), sin(a)) * r;
  vec2 uv = q * 0.5 + 0.5;
  vec3 c = texture(uTex, uv).rgb;
  float vignette = smoothstep(1.25, 0.15, r);
  float glow = smoothstep(0.85, 0.05, r);
  fragColor = vec4(c * vignette + c * glow * 0.25, 1.0);
}
