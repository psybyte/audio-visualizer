#include "viz/Mode.h"

#include "gl/GlExt.h"
#include "gl/Shader.h"
#include "viz/DrawUtil.h"
#include "viz/Palette.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

struct Particle {
  float x = 0.f;
  float y = 0.f;
  float vx = 0.f;
  float vy = 0.f;
  float life = 0.f;
  int band = 0;
};

class ParticleField final : public Mode {
 public:
  ParticleField() {
    particles_.resize(kCount);
    vertices_.reserve(static_cast<size_t>(kCount) * 7);
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    const int stride = 7 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
    glBindVertexArray(0);
    for (int i = 0; i < kCount; ++i) {
      particles_[static_cast<size_t>(i)].life = 0.f;
      particles_[static_cast<size_t>(i)].band = i % AnalysisSnapshot::kBandCount;
    }
  }

  ~ParticleField() override {
    if (vbo_ != 0) {
      glDeleteBuffers(1, &vbo_);
    }
    if (vao_ != 0) {
      glDeleteVertexArrays(1, &vao_);
    }
  }

  const char* name() const override { return kModeNames[5]; }

  void reset() override {
    for (Particle& particle : particles_) {
      particle.life = 0.f;
    }
  }

  void draw(const VizContext& ctx) override {
    if (ctx.audio == nullptr || ctx.particle == nullptr || ctx.batch == nullptr || ctx.basic == nullptr) {
      return;
    }
    const float width = static_cast<float>(ctx.width);
    const float height = static_cast<float>(ctx.height);
    const float dt = std::clamp(ctx.dt, 0.f, 0.05f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    ctx.batch->clear();
    queueBackground(*ctx.batch, ctx.width, ctx.height, ctx.palette);
    ctx.batch->draw(*ctx.basic, width, height);
    ctx.batch->clear();

    vertices_.clear();
    for (Particle& particle : particles_) {
      if (particle.life <= 0.f || particle.x < -30.f || particle.x > width + 30.f || particle.y > height + 30.f) {
        respawn(particle, width, height);
      }
      const float t = static_cast<float>(particle.band) / static_cast<float>(AnalysisSnapshot::kBandCount - 1);
      const float energy = ctx.audio->bands[particle.band];
      const float dx = particle.x - width * 0.5f;
      const float dy = particle.y - height * 0.5f;
      const float distance = std::sqrt(dx * dx + dy * dy) + 1.f;
      particle.vx += (dx / distance) * ctx.audio->bass * 220.f * dt;
      particle.vy += (30.f + t * 80.f + energy * 240.f) * dt;
      const float drag = std::min(0.35f, dt * 1.4f);
      particle.vx *= 1.f - drag;
      particle.vy *= 1.f - drag * 0.35f;
      particle.x += particle.vx * dt;
      particle.y += particle.vy * dt;
      particle.life -= dt * (0.12f + t * 0.38f);
      const Rgb color = paletteColor(ctx.palette, t);
      const float alpha = std::clamp(particle.life * (0.25f + energy), 0.f, 0.95f);
      const float size = std::clamp((4.f + (1.f - t) * 16.f) * (0.45f + energy), 2.f, 42.f);
      push(particle.x, particle.y, size, rgba(color, alpha));
    }

    ctx.particle->use();
    ctx.particle->set2f("uResolution", width, height);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)), vertices_.data(), GL_DYNAMIC_DRAW);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDrawArrays(GL_POINTS, 0, kCount);
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glBindVertexArray(0);
  }

 private:
  static constexpr int kCount = 3200;

  float nextRand() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return static_cast<float>(rng_ >> 8) * (1.f / 16777216.f);
  }

  void respawn(Particle& particle, float width, float height) {
    particle.band = static_cast<int>(nextRand() * static_cast<float>(AnalysisSnapshot::kBandCount - 1));
    particle.x = nextRand() * width;
    particle.y = nextRand() * height;
    const float t = static_cast<float>(particle.band) / static_cast<float>(AnalysisSnapshot::kBandCount - 1);
    particle.vx = (nextRand() - 0.5f) * (20.f + t * 80.f);
    particle.vy = 24.f + t * 110.f + nextRand() * 40.f;
    particle.life = 0.55f + nextRand() * 0.9f;
  }

  void push(float x, float y, float size, Rgba color) {
    vertices_.push_back(x);
    vertices_.push_back(y);
    vertices_.push_back(size);
    vertices_.push_back(color.r);
    vertices_.push_back(color.g);
    vertices_.push_back(color.b);
    vertices_.push_back(color.a);
  }

  std::vector<Particle> particles_;
  std::vector<float> vertices_;
  unsigned int vao_ = 0;
  unsigned int vbo_ = 0;
  unsigned int rng_ = 0x12345678u;
};

}  // namespace

std::unique_ptr<Mode> createParticles() {
  return std::make_unique<ParticleField>();
}
