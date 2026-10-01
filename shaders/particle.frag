#version 330 core
in vec4 vColor;
out vec4 fragColor;
void main() {
  vec2 p = gl_PointCoord * 2.0 - 1.0;
  float d = dot(p, p);
  if (d > 1.0) {
    discard;
  }
  float falloff = smoothstep(1.0, 0.12, d) * vColor.a;
  fragColor = vec4(vColor.rgb * falloff, 1.0);
}
