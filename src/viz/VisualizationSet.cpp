#include "viz/VisualizationSet.h"

#include "gl/GlExt.h"
#include "gl/ScreenQuad.h"
#include "gl/Shader.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <sstream>

namespace {

std::string readText(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return {};
  }
  std::ostringstream output;
  output << input.rdbuf();
  return output.str();
}

std::string displayName(const std::filesystem::path& path) {
  std::string stem = path.stem().string();
  std::size_t index = 0;
  while (index < stem.size() && std::isdigit(static_cast<unsigned char>(stem[index])) != 0) {
    ++index;
  }
  if (index > 0 && index < stem.size() && (stem[index] == '-' || stem[index] == '_' || stem[index] == ' ')) {
    stem = stem.substr(index + 1);
  }
  for (char& character : stem) {
    if (character == '_' || character == '-') {
      character = ' ';
    }
  }
  return stem.empty() ? path.stem().string() : stem;
}

std::string lowerCopy(std::string text) {
  for (char& character : text) {
    character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  }
  return text;
}

std::string shaderHeader(const std::string& paletteSource) {
  return std::string(R"(#version 330 core
in vec2 vUv;
out vec4 fragColor;
uniform vec2 uResolution;
uniform float uTime;
uniform int uPalette;
uniform float uBass;
uniform float uMid;
uniform float uTreble;
uniform float uEnergy;
uniform sampler2D uBands;
uniform sampler2D uWave;
uniform sampler2D uHistory;
uniform float uHistoryHead;
uniform float uHistoryRows;

)") + paletteSource + R"(

float bandAt(float t) {
  return texture(uBands, vec2(clamp(t, 0.0, 1.0), 0.5)).r;
}
float peakAt(float t) {
  return texture(uBands, vec2(clamp(t, 0.0, 1.0), 0.5)).g;
}
vec2 waveAt(float t) {
  return texture(uWave, vec2(clamp(t, 0.0, 1.0), 0.5)).rg;
}
float historyAt(float bandT, float ageT) {
  float row = uHistoryHead - clamp(ageT, 0.0, 1.0) * (uHistoryRows - 1.0);
  float y = (mod(row, uHistoryRows) + 0.5) / uHistoryRows;
  return texture(uHistory, vec2(clamp(bandT, 0.0, 1.0), y)).r;
}

)";
}

std::string builtinId(const std::filesystem::path& path) {
  std::ifstream input(path);
  std::string line;
  while (std::getline(input, line)) {
    const auto first = line.find_first_not_of(" \t\r");
    if (first == std::string::npos || line[first] == '#') {
      continue;
    }
    std::istringstream words(line.substr(first));
    std::string keyword;
    std::string id;
    words >> keyword >> id;
    if (lowerCopy(keyword) == "builtin" && !id.empty()) {
      return lowerCopy(id);
    }
    return {};
  }
  return {};
}

}  // namespace

struct VisualizationSet::Item {
  std::string filename;
  std::string name;
  std::unique_ptr<Mode> mode;
  Shader shader;
  bool shaderMode = false;
};

VisualizationSet::VisualizationSet() = default;

VisualizationSet::~VisualizationSet() {
  items_.clear();
  destroyTexture(bandsTexture_);
  destroyTexture(waveTexture_);
  destroyTexture(historyTexture_);
}

int VisualizationSet::count() const {
  return static_cast<int>(items_.size());
}

void VisualizationSet::load(const std::filesystem::path& directory, const std::filesystem::path& shaderDirectory) {
  items_.clear();
  status_.clear();
  if (!std::filesystem::exists(directory)) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
  }
  if (!std::filesystem::exists(directory)) {
    status_ = "Could not find the visualizations folder.";
    return;
  }

  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const std::string filename = entry.path().filename().string();
    if (filename.empty() || filename[0] == '_' || filename[0] == '.') {
      continue;
    }
    const std::string extension = lowerCopy(entry.path().extension().string());
    if (extension == ".viz" || extension == ".frag") {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());

  bool shaderHeaderReady = false;
  std::string errors;
  for (const auto& file : files) {
    const std::string extension = lowerCopy(file.extension().string());
    try {
      if (extension == ".viz") {
        const std::string id = builtinId(file);
        std::unique_ptr<Mode> mode = id.empty() ? nullptr : createBuiltinVisualization(id);
        if (!mode) {
          throw std::runtime_error(file.filename().string() + " does not name a built-in visualization.");
        }
        Item item;
        item.filename = file.filename().string();
        item.name = displayName(file);
        item.mode = std::move(mode);
        items_.push_back(std::move(item));
        continue;
      }

      if (!shaderHeaderReady) {
        ensureShaderResources(shaderDirectory);
        shaderHeaderReady = shaderResourcesReady_;
      }
      if (!shaderResourcesReady_) {
        throw std::runtime_error("Could not prepare custom shader visualizations.");
      }
      Item item;
      item.filename = file.filename().string();
      item.name = displayName(file);
      item.shaderMode = true;
      item.shader = Shader::fromSources(vertexSource_, shaderHeader(paletteSource_) + readText(file), item.filename);
      items_.push_back(std::move(item));
    } catch (const std::exception& exception) {
      if (!errors.empty()) {
        errors += "\n";
      }
      errors += exception.what();
    }
  }

  if (items_.empty() && errors.empty()) {
    status_ = "No visualization files in " + directory.string();
  } else {
    status_ = errors;
  }
}

const std::string& VisualizationSet::name(int index) const {
  static const std::string kEmpty;
  if (index < 0 || index >= count()) {
    return kEmpty;
  }
  return items_[static_cast<size_t>(index)].name;
}

const std::string& VisualizationSet::filename(int index) const {
  static const std::string kEmpty;
  if (index < 0 || index >= count()) {
    return kEmpty;
  }
  return items_[static_cast<size_t>(index)].filename;
}

int VisualizationSet::indexOfFilename(const std::string& filename) const {
  for (int i = 0; i < count(); ++i) {
    if (items_[static_cast<size_t>(i)].filename == filename) {
      return i;
    }
  }
  return -1;
}

void VisualizationSet::resetAll() {
  for (Item& item : items_) {
    if (item.mode) {
      item.mode->reset();
    }
  }
}

void VisualizationSet::ensureShaderResources(const std::filesystem::path& shaderDirectory) {
  if (shaderResourcesReady_) {
    return;
  }
  vertexSource_ = readText(shaderDirectory / "screen.vert");
  paletteSource_ = readText(shaderDirectory / "palette.glsl");
  if (vertexSource_.empty() || paletteSource_.empty()) {
    return;
  }
  bandsTexture_ = makeRg32fTexture(AnalysisSnapshot::kBandCount, 1);
  waveTexture_ = makeRg32fTexture(AnalysisSnapshot::kWaveCount, 1);
  historyTexture_ = makeR32fTexture(AnalysisSnapshot::kBandCount, kHistoryRows);
  glBindTexture(GL_TEXTURE_2D, historyTexture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  shaderQuad_ = std::make_unique<ScreenQuad>();
  shaderResourcesReady_ = true;
}

void VisualizationSet::uploadAudio(const AnalysisSnapshot& audio) {
  std::vector<float> bands(static_cast<size_t>(AnalysisSnapshot::kBandCount) * 2);
  for (int i = 0; i < AnalysisSnapshot::kBandCount; ++i) {
    bands[static_cast<size_t>(i) * 2] = audio.bands[i];
    bands[static_cast<size_t>(i) * 2 + 1] = audio.peaks[i];
  }
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, bandsTexture_);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, AnalysisSnapshot::kBandCount, 1, GL_RG, GL_FLOAT, bands.data());

  std::vector<float> wave(static_cast<size_t>(AnalysisSnapshot::kWaveCount) * 2);
  for (int i = 0; i < AnalysisSnapshot::kWaveCount; ++i) {
    wave[static_cast<size_t>(i) * 2] = audio.waveL[i];
    wave[static_cast<size_t>(i) * 2 + 1] = audio.waveR[i];
  }
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, waveTexture_);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, AnalysisSnapshot::kWaveCount, 1, GL_RG, GL_FLOAT, wave.data());

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, historyTexture_);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, historyRow_, AnalysisSnapshot::kBandCount, 1, GL_RED, GL_FLOAT, audio.bands);
  historyRow_ = (historyRow_ + 1) % kHistoryRows;
  glActiveTexture(GL_TEXTURE0);
}

void VisualizationSet::drawShader(Item& item, const VizContext& context) {
  if (context.audio == nullptr || shaderQuad_ == nullptr) {
    return;
  }
  uploadAudio(*context.audio);
  item.shader.use();
  item.shader.set2f("uResolution", static_cast<float>(context.width), static_cast<float>(context.height));
  item.shader.set1f("uTime", context.time);
  item.shader.set1i("uPalette", static_cast<int>(context.palette));
  item.shader.set1f("uBass", context.audio->bass);
  item.shader.set1f("uMid", context.audio->mid);
  item.shader.set1f("uTreble", context.audio->treble);
  item.shader.set1f("uEnergy", context.audio->energy);
  item.shader.set1i("uBands", 0);
  item.shader.set1i("uWave", 1);
  item.shader.set1i("uHistory", 2);
  item.shader.set1f("uHistoryHead", static_cast<float>(historyRow_ == 0 ? kHistoryRows - 1 : historyRow_ - 1));
  item.shader.set1f("uHistoryRows", static_cast<float>(kHistoryRows));
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, context.width, context.height);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glClear(GL_COLOR_BUFFER_BIT);
  shaderQuad_->draw();
  glActiveTexture(GL_TEXTURE0);
}

void VisualizationSet::draw(int index, const VizContext& context) {
  if (items_.empty() || context.width < 2 || context.height < 2) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, std::max(context.width, 1), std::max(context.height, 1));
    glClearColor(0.02f, 0.02f, 0.03f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    return;
  }
  const int selected = std::clamp(index, 0, count() - 1);
  Item& item = items_[static_cast<size_t>(selected)];
  if (item.shaderMode) {
    drawShader(item, context);
    return;
  }
  if (item.mode) {
    item.mode->draw(context);
  }
}
