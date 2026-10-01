#version 330 core
in vec2 vUv;
uniform sampler2D uTex;
out vec4 fragColor;
void main() {
  fragColor = vec4(texture(uTex, vUv).rgb, 1.0);
}
