#include "ui/Controls.h"

#include "imgui.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace {

std::string formatTime(float seconds) {
  if (seconds < 0.f) {
    seconds = 0.f;
  }
  const int total = static_cast<int>(seconds + 0.5f);
  char text[16] = {};
  std::snprintf(text, sizeof(text), "%d:%02d", total / 60, total % 60);
  return text;
}

}  // namespace

void drawControls(UiState& state, const UiActions& actions) {
  if (!state.showPanel) {
    return;
  }

  ImGui::SetNextWindowPos(ImVec2(18.f, 18.f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(0.92f);
  ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::PushItemWidth(230.f);

  ImGui::TextDisabled("Source");
  if (ImGui::RadioButton("System", state.source == AudioSource::System)) {
    actions.selectSource(AudioSource::System);
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Microphone", state.source == AudioSource::Microphone)) {
    actions.selectSource(AudioSource::Microphone);
  }
  if (ImGui::RadioButton("File", state.source == AudioSource::File)) {
    actions.selectSource(AudioSource::File);
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Test tone", state.source == AudioSource::Tone)) {
    actions.selectSource(AudioSource::Tone);
  }
  if (state.source == AudioSource::Microphone) {
    const char* preview = state.captureDeviceName.empty() ? "Default" : state.captureDeviceName.c_str();
    static bool comboWasOpen = false;
    const bool comboOpen = ImGui::BeginCombo("Device", preview);
    if (comboOpen && !comboWasOpen && actions.refreshCaptureDevices) {
      actions.refreshCaptureDevices();
    }
    comboWasOpen = comboOpen;
    if (comboOpen) {
      if (state.captureDevices.empty()) {
        ImGui::TextDisabled("No input devices");
      }
      for (int i = 0; i < static_cast<int>(state.captureDevices.size()); ++i) {
        ImGui::PushID(i);
        const bool selected = i == state.captureDeviceIndex;
        if (ImGui::Selectable(state.captureDevices[i].c_str(), selected)) {
          actions.selectCaptureDevice(i);
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
        ImGui::PopID();
      }
      ImGui::EndCombo();
    }
  }
  if (ImGui::Button("Open file...")) {
    actions.browse();
  }
  ImGui::SameLine();
  ImGui::TextWrapped("%s", state.fileName.c_str());

  if (state.source == AudioSource::File && state.fileDuration > 0.f) {
    if (ImGui::Button(state.filePlaying ? "Pause" : "Play")) {
      actions.setPlaying(!state.filePlaying);
    }
    ImGui::SameLine();
    ImGui::Text("%s / %s", formatTime(state.filePosition).c_str(), formatTime(state.fileDuration).c_str());
    static float slider = 0.f;
    static bool dragging = false;
    if (!dragging) {
      slider = std::clamp(state.filePosition, 0.f, state.fileDuration);
    }
    const std::string sliderLabel = formatTime(slider);
    if (ImGui::SliderFloat("##posicion", &slider, 0.f, state.fileDuration, sliderLabel.c_str())) {
      actions.seek(slider);
    }
    dragging = ImGui::IsItemActive();
  }

  ImGui::Separator();
  const int mode = std::clamp(state.mode, 0, kModeCount - 1);
  if (ImGui::BeginCombo("Mode", kModeNames[mode])) {
    for (int i = 0; i < kModeCount; ++i) {
      const bool selected = i == mode;
      if (ImGui::Selectable(kModeNames[i], selected)) {
        state.mode = i;
      }
    }
    ImGui::EndCombo();
  }
  ImGui::SliderFloat("Sensitivity", &state.sensitivity, 0.25f, 3.f, "%.2f");
  ImGui::SliderFloat("Smoothing", &state.smoothing, 0.f, 1.f, "%.2f");
  const int paletteIndex = static_cast<int>(state.palette);
  if (ImGui::BeginCombo("Palette", kPaletteNames[paletteIndex])) {
    for (int i = 0; i < static_cast<int>(Palette::Count); ++i) {
      if (ImGui::Selectable(kPaletteNames[i], i == paletteIndex)) {
        state.palette = static_cast<Palette>(i);
      }
    }
    ImGui::EndCombo();
  }

  ImGui::Separator();
  ImGui::Text("Signal  %d Hz    %.0f fps", state.sampleRate, state.fps);
  if (!state.error.empty()) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.45f, 0.4f, 1.f));
    ImGui::TextWrapped("%s", state.error.c_str());
    ImGui::PopStyleColor();
  }
  ImGui::TextDisabled("1-8 modes    Space play/pause    F11 fullscreen");
  ImGui::TextDisabled("If you hide this panel, press Tab to open it again.");
  if (ImGui::Button("Hide panel")) {
    state.showPanel = false;
  }
  ImGui::PopItemWidth();
  ImGui::End();
}
