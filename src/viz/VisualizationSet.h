#pragma once

#include "viz/Mode.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class VisualizationSet {
 public:
  VisualizationSet();
  ~VisualizationSet();
  VisualizationSet(const VisualizationSet&) = delete;
  VisualizationSet& operator=(const VisualizationSet&) = delete;

  void load(const std::filesystem::path& directory, const std::filesystem::path& shaderDirectory);
  void draw(int index, const VizContext& context);
  void resetAll();

  int count() const;
  const std::string& name(int index) const;
  const std::string& filename(int index) const;
  int indexOfFilename(const std::string& filename) const;
  const std::string& status() const { return status_; }

 private:
  struct Item;
  void drawShader(Item& item, const VizContext& context);
  void ensureShaderResources(const std::filesystem::path& shaderDirectory);
  void uploadAudio(const AnalysisSnapshot& audio);

  std::string vertexSource_;
  std::string paletteSource_;
  std::string status_;
  std::vector<Item> items_;
  std::unique_ptr<class ScreenQuad> shaderQuad_;
  unsigned int bandsTexture_ = 0;
  unsigned int waveTexture_ = 0;
  unsigned int historyTexture_ = 0;
  int historyRow_ = 0;
  bool shaderResourcesReady_ = false;
  static constexpr int kHistoryRows = 256;
};
