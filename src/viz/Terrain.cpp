#include "viz/Mode.h"

#include "gl/GlExt.h"
#include "gl/Math.h"
#include "gl/Shader.h"

#include <cmath>
#include <vector>

namespace {

class TerrainMode final : public Mode {
 public:
  TerrainMode() {
    texture_ = makeR32fTexture(AnalysisSnapshot::kBandCount, kHistory);
    constexpr int kX = 128;
    constexpr int kZ = 96;
    std::vector<float> uv;
    uv.reserve(static_cast<size_t>(kX * kZ * 2));
    for (int z = 0; z < kZ; ++z) {
      for (int x = 0; x < kX; ++x) {
        uv.push_back(static_cast<float>(x) / static_cast<float>(kX - 1));
        uv.push_back(static_cast<float>(z) / static_cast<float>(kZ - 1));
      }
    }
    std::vector<unsigned int> indices;
    indices.reserve(static_cast<size_t>((kX - 1) * (kZ - 1) * 6));
    for (int z = 0; z < kZ - 1; ++z) {
      for (int x = 0; x < kX - 1; ++x) {
        const unsigned int i = static_cast<unsigned int>(z * kX + x);
        indices.push_back(i);
        indices.push_back(i + kX);
        indices.push_back(i + 1);
        indices.push_back(i + 1);
        indices.push_back(i + kX);
        indices.push_back(i + kX + 1);
      }
    }
    indexCount_ = static_cast<int>(indices.size());
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(uv.size() * sizeof(float)), uv.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);
  }

  ~TerrainMode() override {
    if (ebo_ != 0) {
      glDeleteBuffers(1, &ebo_);
    }
    if (vbo_ != 0) {
      glDeleteBuffers(1, &vbo_);
    }
    if (vao_ != 0) {
      glDeleteVertexArrays(1, &vao_);
    }
    destroyTexture(texture_);
  }

  const char* name() const override { return kModeNames[4]; }

  void reset() override {
    std::vector<float> zeros(static_cast<size_t>(AnalysisSnapshot::kBandCount) * kHistory, 0.f);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, AnalysisSnapshot::kBandCount, kHistory, GL_RED, GL_FLOAT, zeros.data());
    row_ = 0;
  }

  void draw(const VizContext& ctx) override {
    if (ctx.audio == nullptr || ctx.terrain == nullptr) {
      return;
    }
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, row_, AnalysisSnapshot::kBandCount, 1, GL_RED, GL_FLOAT, ctx.audio->bands);

    const float aspect = ctx.height > 0 ? static_cast<float>(ctx.width) / static_cast<float>(ctx.height) : 1.f;
    const float angle = ctx.time * 0.18f;
    const float bob = 0.12f * std::sin(ctx.time * 0.45f) + ctx.audio->bass * 0.35f;
    const Vec3 eye{std::sin(angle) * 4.8f, 2.4f + bob, std::cos(angle) * 3.9f};
    const Mat4 view = lookAt(eye, {0.f, 0.25f, 0.f}, {0.f, 1.f, 0.f});
    const Mat4 projection = perspective(50.f * 3.14159265f / 180.f, aspect, 0.08f, 40.f);
    const Mat4 mvp = multiply(projection, view);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    ctx.terrain->use();
    ctx.terrain->setMat4("uMvp", mvp.m);
    ctx.terrain->set1i("uHeight", 0);
    ctx.terrain->set1i("uPalette", static_cast<int>(ctx.palette));
    ctx.terrain->set1f("uNewest", static_cast<float>(row_));
    ctx.terrain->set1f("uRows", static_cast<float>(kHistory));
    ctx.terrain->set1f("uScale", 0.95f + ctx.audio->energy * 0.5f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
    row_ = (row_ + 1) % kHistory;
  }

 private:
  static constexpr int kHistory = 192;
  unsigned int texture_ = 0;
  unsigned int vao_ = 0;
  unsigned int vbo_ = 0;
  unsigned int ebo_ = 0;
  int indexCount_ = 0;
  int row_ = 0;
};

}  // namespace

std::unique_ptr<Mode> createTerrain() {
  return std::make_unique<TerrainMode>();
}
