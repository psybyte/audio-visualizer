#include "analysis/Analyzer.h"
#include "audio/AudioEngine.h"
#include "gl/Batch.h"
#include "gl/GlExt.h"
#include "gl/ScreenQuad.h"
#include "gl/Shader.h"
#include "ui/Controls.h"
#include "ui/FileDialog.h"
#include "viz/Mode.h"
#include "viz/Palette.h"
#include "viz/Trail.h"
#include "viz/VisualizationSet.h"
#include "WinText.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <objbase.h>
#include <shellapi.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

struct ComScope {
  bool owned = false;
  ComScope() {
    const HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    owned = SUCCEEDED(result);
  }
  ~ComScope() {
    if (owned) {
      CoUninitialize();
    }
  }
};

struct Options {
  bool selfTest = false;
  bool probe = false;
  bool gallery = false;
  bool help = false;
  bool checkTransport = false;
  AudioSource source = AudioSource::System;
  bool sourceChosen = false;
  std::wstring file;
  std::wstring galleryDir;
};

struct WindowedPlacement {
  bool fullscreen = false;
  int x = 80;
  int y = 60;
  int width = 1400;
  int height = 840;
};

void attachParentConsole() {
  wchar_t* logPath = nullptr;
  size_t logLength = 0;
  if (_wdupenv_s(&logPath, &logLength, L"AUDIO_VIZ_LOG") == 0 && logPath != nullptr && logPath[0] != L'\0') {
    FILE* log = nullptr;
    _wfreopen_s(&log, logPath, L"w", stdout);
    _wfreopen_s(&log, logPath, L"a", stderr);
    free(logPath);
    std::ios::sync_with_stdio();
    return;
  }
  free(logPath);
  if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
    return;
  }
  FILE* stream = nullptr;
  freopen_s(&stream, "CONOUT$", "w", stdout);
  freopen_s(&stream, "CONOUT$", "w", stderr);
  freopen_s(&stream, "CONIN$", "r", stdin);
  std::ios::sync_with_stdio();
}

void showError(const std::string& message) {
  std::cerr << message << '\n';
  MessageBoxW(nullptr, wideFromUtf8(message).c_str(), L"Audio Visualizer", MB_OK | MB_ICONERROR);
}

std::filesystem::path executableDirectory() {
  wchar_t buffer[MAX_PATH] = {};
  const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
  if (length == 0 || length >= MAX_PATH) {
    return std::filesystem::current_path();
  }
  return std::filesystem::path(buffer).parent_path();
}

void printUsage() {
  std::cout << "Audio Visualizer\n"
            << "  --self-test          Check the FFT and exit\n"
            << "  --probe              Test tone, system, microphone, and an optional file\n"
            << "  --file <path>        Play a wav, mp3, or flac file\n"
            << "  --source system|mic|tone|file\n"
            << "  --gallery <folder>   Save one image per mode and exit\n";
}

Options parseArguments(int argc, wchar_t** argv) {
  Options options;
  for (int i = 1; i < argc; ++i) {
    const std::wstring arg = argv[i];
    auto requireValue = [&](const wchar_t* name) -> std::wstring {
      if (i + 1 >= argc) {
        throw std::runtime_error(std::string("Missing a value for ") + utf8FromWide(name));
      }
      return argv[++i];
    };
    if (arg == L"--help" || arg == L"-h" || arg == L"/?") {
      options.help = true;
    } else if (arg == L"--self-test") {
      options.selfTest = true;
    } else if (arg == L"--probe") {
      options.probe = true;
    } else if (arg == L"--check-transport") {
      options.checkTransport = true;
    } else if (arg == L"--gallery") {
      options.gallery = true;
      options.galleryDir = requireValue(L"--gallery");
    } else if (arg == L"--file") {
      options.file = requireValue(L"--file");
    } else if (arg == L"--source") {
      const std::wstring value = requireValue(L"--source");
      options.sourceChosen = true;
      if (value == L"system") {
        options.source = AudioSource::System;
      } else if (value == L"mic" || value == L"microphone") {
        options.source = AudioSource::Microphone;
      } else if (value == L"tone") {
        options.source = AudioSource::Tone;
      } else if (value == L"file") {
        options.source = AudioSource::File;
      } else {
        throw std::runtime_error("Unknown source. Use system, mic, tone, or file.");
      }
    } else {
      throw std::runtime_error("Unknown argument: " + utf8FromWide(arg));
    }
  }
  return options;
}

float signalPeak(AudioEngine& audio) {
  std::vector<float> samples(static_cast<size_t>(AnalysisSnapshot::kFftSize) * 2, 0.f);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (audio.framesWritten() < static_cast<uint64_t>(AnalysisSnapshot::kFftSize) && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  audio.readLatest(samples.data(), AnalysisSnapshot::kFftSize);
  float peak = 0.f;
  for (float sample : samples) {
    peak = std::max(peak, std::fabs(sample));
  }
  return peak;
}

int runProbe(const std::wstring& file) {
  AudioEngine audio;
  int importantFailures = 0;
  auto report = [&](const char* name, bool opened, bool requireSignal) {
    std::cout << name << ": " << (opened ? "open" : audio.lastError()) << "  " << audio.sampleRate() << " Hz\n";
    if (!opened) {
      if (requireSignal) {
        ++importantFailures;
      }
      return;
    }
    const float peak = signalPeak(audio);
    std::cout << "  peak " << peak << '\n';
    if (requireSignal && peak < 0.02f) {
      std::cout << "  no audio samples\n";
      ++importantFailures;
    }
    audio.stop();
  };

  report("Tone", audio.start(AudioSource::Tone), true);
  report("System", audio.start(AudioSource::System), false);
  report("Microphone", audio.start(AudioSource::Microphone), false);
  if (audio.source() == AudioSource::Microphone) {
    std::cout << "  device " << audio.captureDeviceName() << '\n';
  }
  if (!file.empty()) {
    report("File", audio.startFile(file), true);
  }
  return importantFailures == 0 ? 0 : 1;
}

bool keyPressed(GLFWwindow* window, int key) {
  static bool previous[400] = {};
  if (key < 0 || key >= 400) {
    return false;
  }
  const bool down = glfwGetKey(window, key) == GLFW_PRESS;
  const bool edge = down && !previous[key];
  previous[key] = down;
  return edge;
}

void toggleFullscreen(GLFWwindow* window, WindowedPlacement& placement) {
  if (!placement.fullscreen) {
    glfwGetWindowPos(window, &placement.x, &placement.y);
    glfwGetWindowSize(window, &placement.width, &placement.height);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    placement.fullscreen = true;
  } else {
    glfwSetWindowMonitor(window, nullptr, placement.x, placement.y, placement.width, placement.height, 0);
    placement.fullscreen = false;
  }
}

std::string fileLabel(const std::wstring& path) {
  if (path.empty()) {
    return "No file";
  }
  const auto utf8 = std::filesystem::path(path).filename().u8string();
  return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}

double saveBmp(const std::filesystem::path& path, int width, int height) {
  std::vector<unsigned char> rgba(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
  const int stride = (width * 3 + 3) & ~3;
  std::vector<unsigned char> pixels(static_cast<size_t>(stride) * static_cast<size_t>(height), 0);
  double sum = 0.0;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const unsigned char* source = rgba.data() + (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)) * 4;
      unsigned char* destination = pixels.data() + static_cast<size_t>(y) * static_cast<size_t>(stride) + static_cast<size_t>(x) * 3;
      destination[0] = source[2];
      destination[1] = source[1];
      destination[2] = source[0];
      sum += static_cast<double>(source[0]) + source[1] + source[2];
    }
  }
  const uint32_t fileSize = 54u + static_cast<uint32_t>(pixels.size());
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    throw std::runtime_error("Could not write " + path.string());
  }
  unsigned char header[54] = {};
  header[0] = 'B';
  header[1] = 'M';
  header[2] = static_cast<unsigned char>(fileSize);
  header[3] = static_cast<unsigned char>(fileSize >> 8);
  header[4] = static_cast<unsigned char>(fileSize >> 16);
  header[5] = static_cast<unsigned char>(fileSize >> 24);
  header[10] = 54;
  header[14] = 40;
  header[18] = static_cast<unsigned char>(width);
  header[19] = static_cast<unsigned char>(width >> 8);
  header[20] = static_cast<unsigned char>(width >> 16);
  header[21] = static_cast<unsigned char>(width >> 24);
  header[22] = static_cast<unsigned char>(height);
  header[23] = static_cast<unsigned char>(height >> 8);
  header[24] = static_cast<unsigned char>(height >> 16);
  header[25] = static_cast<unsigned char>(height >> 24);
  header[26] = 1;
  header[28] = 24;
  output.write(reinterpret_cast<char*>(header), 54);
  output.write(reinterpret_cast<char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
  return sum / (static_cast<double>(width) * static_cast<double>(height) * 3.0);
}

struct Scene {
  Shader basic;
  Shader fade;
  Shader blit;
  Shader spectrogram;
  Shader terrain;
  Shader particle;
  Shader kaleidoscope;
  Batch batch;
  ScreenQuad quad;
  std::unique_ptr<Trail> trail;
  VisualizationSet visuals;

  Scene()
      : basic(Shader::fromFiles(executableDirectory() / "shaders" / "basic.vert", executableDirectory() / "shaders" / "basic.frag")),
        fade(Shader::fromFiles(executableDirectory() / "shaders" / "screen.vert", executableDirectory() / "shaders" / "trail.frag")),
        blit(Shader::fromFiles(executableDirectory() / "shaders" / "screen.vert", executableDirectory() / "shaders" / "blit.frag")),
        spectrogram(Shader::fromFiles(executableDirectory() / "shaders" / "screen.vert", executableDirectory() / "shaders" / "spectrogram.frag")),
        terrain(Shader::fromFiles(executableDirectory() / "shaders" / "terrain.vert", executableDirectory() / "shaders" / "terrain.frag")),
        particle(Shader::fromFiles(executableDirectory() / "shaders" / "particle.vert", executableDirectory() / "shaders" / "particle.frag")),
        kaleidoscope(Shader::fromFiles(executableDirectory() / "shaders" / "screen.vert", executableDirectory() / "shaders" / "kaleidoscope.frag")),
        batch(),
        quad(),
        trail(std::make_unique<Trail>(fade, blit)) {
    const auto root = executableDirectory();
    visuals.load(root / "visualizations", root / "shaders");
  }
};

VizContext makeContext(Scene& scene, const AnalysisSnapshot& audio, const UiState& ui, int width, int height, float time, float dt) {
  VizContext context;
  context.width = width;
  context.height = height;
  context.time = time;
  context.dt = dt;
  context.audio = &audio;
  context.palette = ui.palette;
  context.batch = &scene.batch;
  context.trail = scene.trail.get();
  context.quad = &scene.quad;
  context.basic = &scene.basic;
  context.spectrogram = &scene.spectrogram;
  context.terrain = &scene.terrain;
  context.particle = &scene.particle;
  context.kaleidoscope = &scene.kaleidoscope;
  return context;
}

void drawScene(Scene& scene, const VizContext& context, int mode) {
  const Rgb background = backgroundBottom(context.palette);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, context.width, context.height);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glClearColor(background.r, background.g, background.b, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  scene.visuals.draw(mode, context);
}

void fillModeList(UiState& ui, const VisualizationSet& visuals) {
  ui.modeNames.clear();
  ui.modeNames.reserve(static_cast<size_t>(visuals.count()));
  for (int i = 0; i < visuals.count(); ++i) {
    ui.modeNames.push_back(visuals.name(i));
  }
  ui.visualizationNote = visuals.status();
  if (visuals.count() == 0) {
    ui.mode = 0;
  } else if (ui.mode >= visuals.count()) {
    ui.mode = visuals.count() - 1;
  }
}

int runGallery(GLFWwindow* window, AudioEngine& audio, Scene& scene, const std::filesystem::path& directory) {
  std::vector<float> samples(static_cast<size_t>(AnalysisSnapshot::kFftSize) * 2, 0.f);
  Analyzer analyzer;
  AnalysisSnapshot snapshot;
  UiState ui;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (audio.framesWritten() < static_cast<uint64_t>(AnalysisSnapshot::kFftSize) && std::chrono::steady_clock::now() < deadline) {
    glfwPollEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  const int modeCount = scene.visuals.count();
  std::vector<std::string> names;
  names.reserve(static_cast<size_t>(modeCount));
  for (int mode = 0; mode < modeCount; ++mode) {
    std::string file = mode < 9 ? "0" : "";
    file += std::to_string(mode + 1);
    file += "-";
    for (char letter : scene.visuals.name(mode)) {
      const unsigned char character = static_cast<unsigned char>(letter);
      file.push_back(static_cast<char>(character >= 'A' && character <= 'Z' ? character - 'A' + 'a' : character));
    }
    file += ".bmp";
    names.push_back(std::move(file));
  }
  int failures = 0;
  VisualSettings settings;
  settings.smoothing = 0.35f;
  for (int mode = 0; mode < modeCount; ++mode) {
    analyzer.reset();
    int width = 0;
    int height = 0;
    GLenum error = GL_NO_ERROR;
    double mean = 0.0;
    for (int frame = 0; frame < 180; ++frame) {
      glfwPollEvents();
      if (glfwWindowShouldClose(window)) {
        return 1;
      }
      glfwGetFramebufferSize(window, &width, &height);
      if (width < 2 || height < 2) {
        continue;
      }
      audio.readLatest(samples.data(), AnalysisSnapshot::kFftSize);
      analyzer.process(samples.data(), audio.sampleRate(), 1.f / 60.f, settings, snapshot);
      ui.palette = Palette::Neon;
      while (glGetError() != GL_NO_ERROR) {
      }
      const VizContext context = makeContext(scene, snapshot, ui, width, height, frame / 60.f, 1.f / 60.f);
      drawScene(scene, context, mode);
      const GLenum frameError = glGetError();
      if (frameError != GL_NO_ERROR) {
        error = frameError;
      }
      if (frame == 179) {
        glFinish();
        mean = saveBmp(directory / names[mode], width, height);
      }
      glfwSwapBuffers(window);
    }
    std::cout << names[mode] << " mean " << mean;
    if (error != GL_NO_ERROR) {
      std::cout << "  GL " << error;
    }
    std::cout << '\n';
    if (mean < 1.5 || error != GL_NO_ERROR) {
      ++failures;
    }
  }
  return failures == 0 ? 0 : 1;
}

int runInteractive(GLFWwindow* window, AudioEngine& audio, Scene& scene) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 10.f;
  style.FrameRounding = 5.f;
  style.GrabRounding = 4.f;
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.08f, 0.94f);
  style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.05f, 0.14f, 1.f);
  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.08f, 0.30f, 1.f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.18f, 0.12f, 0.32f, 1.f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.18f, 0.50f, 1.f);
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.45f, 0.62f, 1.f);
  style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.78f, 0.95f, 1.f);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.9f, 1.f, 1.f);
  style.Colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.9f, 1.f, 1.f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.14f, 0.40f, 1.f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.32f, 0.20f, 0.55f, 1.f);
  style.Colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.10f, 0.18f, 1.f);
  float scaleX = 1.f;
  float scaleY = 1.f;
  glfwGetWindowContentScale(window, &scaleX, &scaleY);
  const float dpi = std::max(scaleX, 1.f);
  style.FontScaleDpi = dpi;
  style.ScaleAllSizes(dpi);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  std::vector<float> samples(static_cast<size_t>(AnalysisSnapshot::kFftSize) * 2, 0.f);
  Analyzer analyzer;
  AnalysisSnapshot snapshot;
  UiState ui;
  ui.sensitivity = 1.45f;
  ui.smoothing = 0.58f;
  WindowedPlacement placement;
  uint64_t seenSession = audio.session();
  double lastTime = glfwGetTime();
  float fps = 60.f;

  auto browse = [&]() {
    if (const auto path = openAudioFileDialog(glfwGetWin32Window(window))) {
      audio.startFile(*path);
    }
  };
  UiActions actions;
  actions.browse = browse;
  actions.setPlaying = [&](bool playing) { audio.setFilePlaying(playing); };
  actions.seek = [&](float seconds) { audio.seekFileSeconds(seconds); };
  actions.refreshVisualizations = [&]() {
    const std::string keep = scene.visuals.filename(ui.mode);
    const auto root = executableDirectory();
    scene.visuals.load(root / "visualizations", root / "shaders");
    const int found = scene.visuals.indexOfFilename(keep);
    ui.mode = found >= 0 ? found : 0;
    fillModeList(ui, scene.visuals);
  };
  actions.refreshCaptureDevices = [&]() {
    audio.refreshCaptureDevices();
    ui.captureDevices = audio.captureDeviceNames();
    ui.captureDeviceIndex = audio.captureDeviceIndex();
    ui.captureDeviceName = audio.captureDeviceName();
  };
  actions.selectCaptureDevice = [&](int index) { audio.setCaptureDevice(index); };
  actions.selectSource = [&](AudioSource source) {
    if (source == AudioSource::File) {
      if (audio.filePath().empty()) {
        browse();
      } else {
        audio.startFile(audio.filePath());
      }
    } else {
      audio.start(source);
    }
  };

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    const double now = glfwGetTime();
    const float dt = std::clamp(static_cast<float>(now - lastTime), 0.f, 0.05f);
    lastTime = now;
    if (dt > 0.f) {
      fps = fps * 0.9f + (1.f / dt) * 0.1f;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    const bool captureKeys = ImGui::GetIO().WantCaptureKeyboard;

    if (keyPressed(window, GLFW_KEY_TAB)) {
      ui.showPanel = !ui.showPanel;
    }
    if (keyPressed(window, GLFW_KEY_F11)) {
      toggleFullscreen(window, placement);
    }
    if (keyPressed(window, GLFW_KEY_ESCAPE)) {
      if (placement.fullscreen) {
        toggleFullscreen(window, placement);
      } else {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
      }
    }
    const int modeCount = scene.visuals.count();
    if (!captureKeys && modeCount > 0) {
      const int numberKeys = std::min(modeCount, 9);
      for (int i = 0; i < numberKeys; ++i) {
        if (keyPressed(window, GLFW_KEY_1 + i)) {
          ui.mode = i;
        }
      }
      if (modeCount > 9 && keyPressed(window, GLFW_KEY_0)) {
        ui.mode = 9;
      }
      if (keyPressed(window, GLFW_KEY_LEFT) || keyPressed(window, GLFW_KEY_LEFT_BRACKET)) {
        ui.mode = (ui.mode + modeCount - 1) % modeCount;
      }
      if (keyPressed(window, GLFW_KEY_RIGHT) || keyPressed(window, GLFW_KEY_RIGHT_BRACKET)) {
        ui.mode = (ui.mode + 1) % modeCount;
      }
      if (audio.source() == AudioSource::File && keyPressed(window, GLFW_KEY_SPACE)) {
        audio.setFilePlaying(!audio.filePlaying());
      }
    }

    audio.updateFilePlayback();
    ui.source = audio.source();
    ui.filePlaying = audio.filePlaying();
    ui.filePosition = static_cast<float>(audio.filePositionSeconds());
    ui.fileDuration = static_cast<float>(audio.fileDurationSeconds());
    ui.fileName = fileLabel(audio.filePath());
    fillModeList(ui, scene.visuals);
    ui.captureDevices = audio.captureDeviceNames();
    ui.captureDeviceIndex = audio.captureDeviceIndex();
    ui.captureDeviceName = audio.captureDeviceName();
    ui.error = audio.lastError();
    ui.sampleRate = audio.sampleRate();
    ui.fps = fps;
    drawControls(ui, actions);

    if (audio.session() != seenSession) {
      analyzer.reset();
      scene.trail->clear();
      scene.visuals.resetAll();
      seenSession = audio.session();
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    if (width >= 2 && height >= 2) {
      audio.readLatest(samples.data(), AnalysisSnapshot::kFftSize);
      VisualSettings settings;
      settings.sensitivity = ui.sensitivity;
      settings.smoothing = ui.smoothing;
      settings.palette = ui.palette;
      analyzer.process(samples.data(), audio.sampleRate(), dt, settings, snapshot);
      const VizContext context = makeContext(scene, snapshot, ui, width, height, static_cast<float>(now), dt);
      drawScene(scene, context, ui.mode);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  return 0;
}

int runWindow(const Options& options) {
  AudioEngine audio;
  if (options.gallery) {
    audio.start(AudioSource::Tone);
  } else if (!options.file.empty() && (!options.sourceChosen || options.source == AudioSource::File)) {
    audio.startFile(options.file);
  } else {
    audio.start(options.source);
  }

  glfwSetErrorCallback([](int code, const char* description) {
    std::cerr << "GLFW " << code << ": " << (description != nullptr ? description : "") << '\n';
  });
  if (!glfwInit()) {
    throw std::runtime_error("Could not start the window.");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_DEPTH_BITS, 24);
  glfwWindowHint(GLFW_SAMPLES, 0);
  GLFWwindow* window = glfwCreateWindow(1400, 840, "Audio Visualizer", nullptr, nullptr);
  if (window == nullptr) {
    glfwTerminate();
    throw std::runtime_error("Could not create the OpenGL 3.3 window.");
  }
  glfwSetWindowSizeLimits(window, 960, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);
  glfwSetWindowUserPointer(window, &audio);
  glfwSetDropCallback(window, [](GLFWwindow* dropped, int count, const char** paths) {
    if (count > 0 && paths != nullptr && paths[0] != nullptr) {
      auto* engine = static_cast<AudioEngine*>(glfwGetWindowUserPointer(dropped));
      engine->startFile(wideFromUtf8(paths[0]));
    }
  });
  glfwMakeContextCurrent(window);
  glfwSwapInterval(options.gallery ? 0 : 1);
  initGlExtensions();

  int code = 0;
  {
    Scene scene;
    if (options.gallery) {
      std::filesystem::create_directories(options.galleryDir);
      code = runGallery(window, audio, scene, options.galleryDir);
    } else {
      code = runInteractive(window, audio, scene);
    }
  }

  audio.stop();
  glfwDestroyWindow(window);
  glfwTerminate();
  return code;
}

int run(int argc, wchar_t** argv) {
  const Options options = parseArguments(argc, argv);
  if (options.help) {
    printUsage();
    return 0;
  }
  std::string error;
  if (!Analyzer::selfTest(error)) {
    showError(error);
    return 1;
  }
  if (options.selfTest) {
    std::cout << "FFT check passed.\n";
    return 0;
  }

  ComScope com;
  (void)com;
  if (options.probe) {
    return runProbe(options.file);
  }
  if (options.checkTransport) {
    if (options.file.empty()) {
      throw std::runtime_error("Pass a file with --file.");
    }
    AudioEngine audio;
    if (!audio.startFile(options.file)) {
      throw std::runtime_error(audio.lastError());
    }
    const double duration = audio.fileDurationSeconds();
    if (!(duration > 0.5) || !audio.filePlaying()) {
      throw std::runtime_error("The file did not start playing.");
    }
    audio.seekFileSeconds(1.0);
    const double afterSeek = audio.filePositionSeconds();
    if (afterSeek < 0.95 || afterSeek > 1.05) {
      throw std::runtime_error("The seek did not land on second 1.");
    }
    audio.setFilePlaying(false);
    const double pausedAt = audio.filePositionSeconds();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (audio.filePlaying() || std::abs(audio.filePositionSeconds() - pausedAt) > 0.02) {
      throw std::runtime_error("Pause did not stop playback.");
    }
    audio.setFilePlaying(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (!audio.filePlaying() || audio.filePositionSeconds() <= pausedAt) {
      throw std::runtime_error("Play did not resume the file.");
    }
    std::cout << "Transport ok. Duration " << duration << " s\n";
    return 0;
  }
  return runWindow(options);
}

}  // namespace

int main() {
  attachParentConsole();
  SetConsoleOutputCP(CP_UTF8);
  int argc = 0;
  wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  int code = 1;
  try {
    code = run(argc, argv);
  } catch (const std::exception& exception) {
    showError(exception.what());
    code = 1;
  }
  if (argv != nullptr) {
    LocalFree(argv);
  }
  return code;
}
