#include "viz/Mode.h"

#include <string>

std::unique_ptr<Mode> createBuiltinVisualization(const std::string& id) {
  if (id == "bars") return createSpectrumBars();
  if (id == "ring") return createRadialRing();
  if (id == "oscilloscope") return createOscilloscope();
  if (id == "spectrogram") return createSpectrogram();
  if (id == "terrain") return createTerrain();
  if (id == "particles") return createParticles();
  if (id == "lissajous") return createLissajous();
  if (id == "kaleidoscope") return createKaleidoscope();
  if (id == "nebula") return createNebula();
  if (id == "skyline") return createSkyline();
  if (id == "radar") return createRadar();
  if (id == "ripples") return createRipples();
  if (id == "stereo") return createStereoField();
  if (id == "helix") return createHelix();
  return nullptr;
}
