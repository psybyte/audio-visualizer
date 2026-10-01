#version 330 core
in vec2 vUv;
uniform sampler2D uTex;
uniform float uFade;
out vec4 fragColor;
void main() {
  vec4 c = texture(uTex, vUv);
  fragColor = vec4(c.rgb * uFade, 1.0);
}
