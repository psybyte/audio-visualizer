vec3 ramp4(float t, vec3 c0, vec3 c1, vec3 c2, vec3 c3) {
  t = clamp(t, 0.0, 1.0);
  if (t < 0.35) {
    return mix(c0, c1, smoothstep(0.0, 1.0, t / 0.35));
  }
  if (t < 0.70) {
    return mix(c1, c2, smoothstep(0.0, 1.0, (t - 0.35) / 0.35));
  }
  return mix(c2, c3, smoothstep(0.0, 1.0, (t - 0.70) / 0.30));
}

vec3 paletteColor(int palette, float t) {
  if (palette == 1) {
    return ramp4(t, vec3(0.00, 0.10, 0.03), vec3(0.05, 0.55, 0.16), vec3(0.45, 0.95, 0.30), vec3(0.80, 1.00, 0.62));
  }
  if (palette == 2) {
    return ramp4(t, vec3(0.08, 0.00, 0.03), vec3(0.72, 0.06, 0.08), vec3(1.00, 0.42, 0.05), vec3(1.00, 0.95, 0.72));
  }
  if (palette == 3) {
    return ramp4(t, vec3(0.02, 0.06, 0.22), vec3(0.10, 0.32, 0.90), vec3(0.35, 0.86, 1.00), vec3(0.92, 1.00, 1.00));
  }
  return ramp4(t, vec3(0.20, 0.02, 0.48), vec3(0.95, 0.10, 0.55), vec3(0.15, 0.82, 1.00), vec3(0.88, 1.00, 1.00));
}
