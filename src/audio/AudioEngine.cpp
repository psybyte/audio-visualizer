#include "audio/AudioEngine.h"

#include "miniaudio.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <vector>

namespace {

constexpr uint32_t kRingFrames = 1u << 16;

std::string describeFailure(const char* message, ma_result result) {
  return std::string(message) + " (" + ma_result_description(result) + ")";
}

}  // namespace

struct AudioEngine::Impl {
  ma_device device{};
  ma_decoder decoder{};
  bool deviceOpen = false;
  bool decoderOpen = false;
  AudioSource source = AudioSource::Tone;
  std::atomic<int> sampleRate{48000};
  std::atomic<uint64_t> framesWritten{0};
  std::atomic<uint64_t> session{0};
  std::atomic<uint64_t> cursor{0};
  std::atomic<bool> playing{false};
  std::atomic<bool> reachedEnd{false};
  uint64_t fileLength = 0;
  uint64_t toneSample = 0;
  ma_context context{};
  bool contextReady = false;
  struct ListedCaptureDevice {
    std::string label;
    ma_device_id id{};
    bool isDefault = false;
  };
  std::vector<ListedCaptureDevice> captureList;
  ma_device_id selectedCaptureId{};
  bool hasCaptureSelection = false;
  std::string activeCaptureName;
  std::string error;
  std::wstring filePath;
  std::vector<float> samples;
  uint64_t writeIndex = 0;
  mutable std::mutex mutex;
  std::mutex decoderMutex;

  Impl() : samples(static_cast<size_t>(kRingFrames) * 2, 0.f) {
    ma_backend backends[] = {ma_backend_wasapi};
    const ma_context_config config = ma_context_config_init();
    contextReady = ma_context_init(backends, 1, &config, &context) == MA_SUCCESS;
  }

  ~Impl() {
    closeDevice();
    if (contextReady) {
      ma_context_uninit(&context);
      contextReady = false;
    }
  }

  void clearRing() {
    std::lock_guard lock(mutex);
    std::fill(samples.begin(), samples.end(), 0.f);
    writeIndex = 0;
    framesWritten.store(0, std::memory_order_relaxed);
  }

  void push(const float* interleaved, uint32_t frames, uint32_t channels) {
    if (interleaved == nullptr || frames == 0 || channels == 0) {
      return;
    }
    std::lock_guard lock(mutex);
    for (uint32_t i = 0; i < frames; ++i) {
      const float left = interleaved[i * channels];
      const float right = channels > 1 ? interleaved[i * channels + 1] : left;
      const uint32_t index = static_cast<uint32_t>(writeIndex & (kRingFrames - 1));
      samples[static_cast<size_t>(index) * 2] = left;
      samples[static_cast<size_t>(index) * 2 + 1] = right;
      ++writeIndex;
    }
    framesWritten.fetch_add(frames, std::memory_order_relaxed);
  }

  void readLatest(float* destination, uint32_t frames) const {
    std::lock_guard lock(mutex);
    for (uint32_t i = 0; i < frames; ++i) {
      const int64_t sourceFrame = static_cast<int64_t>(writeIndex) - static_cast<int64_t>(frames) + static_cast<int64_t>(i);
      if (sourceFrame < 0) {
        destination[i * 2] = 0.f;
        destination[i * 2 + 1] = 0.f;
        continue;
      }
      const uint32_t index = static_cast<uint32_t>(static_cast<uint64_t>(sourceFrame) & (kRingFrames - 1));
      destination[i * 2] = samples[static_cast<size_t>(index) * 2];
      destination[i * 2 + 1] = samples[static_cast<size_t>(index) * 2 + 1];
    }
  }

  void generateTone(float* destination, ma_uint32 frameCount) {
    constexpr double kPi = 3.14159265358979323846;
    const double rate = sampleRate.load() > 0 ? static_cast<double>(sampleRate.load()) : 48000.0;
    for (ma_uint32 i = 0; i < frameCount; ++i) {
      const double t = static_cast<double>(toneSample++) / rate;
      const float envelope = static_cast<float>(0.5 + 0.5 * std::sin(2.0 * kPi * 2.0 * t));
      const float pulse = envelope * envelope * envelope * envelope;
      float bass = 0.f;
      for (int harmonic = 1; harmonic <= 5; ++harmonic) {
        bass += static_cast<float>(std::sin(2.0 * kPi * 55.0 * harmonic * t) / harmonic);
      }
      bass *= 0.22f * (0.30f + 0.70f * pulse);
      const float wobble = static_cast<float>(0.5 + 0.5 * std::sin(2.0 * kPi * 0.07 * t));
      const float mid = static_cast<float>(std::sin(2.0 * kPi * 220.0 * t)) * (0.16f * (1.f - wobble)) +
                        static_cast<float>(std::sin(2.0 * kPi * 277.18 * t)) * 0.12f +
                        static_cast<float>(std::sin(2.0 * kPi * 329.63 * t)) * (0.15f * wobble) +
                        static_cast<float>(std::sin(2.0 * kPi * 440.0 * t)) * 0.07f;
      const double sweep = 1200.0 + 700.0 * std::sin(2.0 * kPi * 0.04 * t);
      const float high = static_cast<float>(std::sin(2.0 * kPi * sweep * t)) * 0.045f *
                         (0.45f + 0.55f * static_cast<float>(std::sin(2.0 * kPi * 5.0 * t)));
      const float side = static_cast<float>(std::sin(2.0 * kPi * 196.0 * t)) * 0.06f;
      const float left = std::clamp(bass + mid + high + side, -0.98f, 0.98f);
      const float right = std::clamp(bass + mid + high - side * 0.6f + static_cast<float>(std::sin(2.0 * kPi * 392.0 * t)) * 0.04f,
                                      -0.98f, 0.98f);
      destination[i * 2] = left;
      destination[i * 2 + 1] = right;
    }
  }

  void onAudio(void* output, const void* input, ma_uint32 frameCount) {
    if (frameCount == 0) {
      return;
    }
    if (source == AudioSource::Tone) {
      if (output == nullptr) {
        return;
      }
      generateTone(static_cast<float*>(output), frameCount);
      push(static_cast<float*>(output), frameCount, 2);
      return;
    }
    if (source == AudioSource::File) {
      if (output == nullptr) {
        return;
      }
      auto* destination = static_cast<float*>(output);
      std::fill(destination, destination + static_cast<size_t>(frameCount) * 2, 0.f);
      std::lock_guard lock(decoderMutex);
      const uint64_t position = cursor.load(std::memory_order_relaxed);
      if (fileLength > 0 && position >= fileLength) {
        reachedEnd.store(true, std::memory_order_relaxed);
        return;
      }
      ma_uint32 toRead = frameCount;
      if (fileLength > 0) {
        const uint64_t remaining = fileLength - position;
        if (remaining < toRead) {
          toRead = static_cast<ma_uint32>(remaining);
        }
      }
      ma_uint64 framesRead = 0;
      if (toRead > 0) {
        ma_decoder_read_pcm_frames(&decoder, destination, toRead, &framesRead);
      }
      cursor.store(position + framesRead, std::memory_order_relaxed);
      if (framesRead < toRead || (fileLength > 0 && position + framesRead >= fileLength)) {
        reachedEnd.store(true, std::memory_order_relaxed);
      }
      push(destination, frameCount, 2);
      return;
    }
    if (input == nullptr) {
      return;
    }
    push(static_cast<const float*>(input), frameCount, 2);
  }

  void closeDevice() {
    if (deviceOpen) {
      ma_device_uninit(&device);
      deviceOpen = false;
    }
    if (decoderOpen) {
      ma_decoder_uninit(&decoder);
      decoderOpen = false;
    }
    playing.store(false, std::memory_order_relaxed);
    reachedEnd.store(false, std::memory_order_relaxed);
    cursor.store(0, std::memory_order_relaxed);
    fileLength = 0;
  }

  void previewAt(uint64_t frame) {
    constexpr uint32_t kPreview = 4096;
    std::vector<float> window(static_cast<size_t>(kPreview) * 2, 0.f);
    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&decoder, window.data(), kPreview, &framesRead);
    if (framesRead > 0) {
      push(window.data(), static_cast<uint32_t>(framesRead), 2);
    }
    ma_decoder_seek_to_pcm_frame(&decoder, frame);
  }

  bool openPlayback(AudioSource nextSource, int rate) {
    source = nextSource;
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = static_cast<ma_uint32>(rate);
    config.periodSizeInFrames = 512;
    config.performanceProfile = ma_performance_profile_low_latency;
    config.dataCallback = &AudioEngine::Impl::callback;
    config.pUserData = this;
    const ma_result result = ma_device_init(contextReady ? &context : nullptr, &config, &device);
    if (result != MA_SUCCESS) {
      error = describeFailure("Could not open the audio output", result);
      return false;
    }
    deviceOpen = true;
    sampleRate.store(static_cast<int>(device.sampleRate));
    const ma_result started = ma_device_start(&device);
    if (started != MA_SUCCESS) {
      error = describeFailure("Could not start the audio output", started);
      closeDevice();
      return false;
    }
    return true;
  }

  void refreshCaptureDevices() {
    captureList.clear();
    if (!contextReady) {
      return;
    }
    ma_device_info* playbackInfos = nullptr;
    ma_uint32 playbackCount = 0;
    ma_device_info* captureInfos = nullptr;
    ma_uint32 captureCount = 0;
    if (ma_context_get_devices(&context, &playbackInfos, &playbackCount, &captureInfos, &captureCount) != MA_SUCCESS) {
      return;
    }
    for (ma_uint32 i = 0; i < captureCount; ++i) {
      ListedCaptureDevice item;
      item.id = captureInfos[i].id;
      item.isDefault = captureInfos[i].isDefault == MA_TRUE;
      item.label = captureInfos[i].name[0] != '\0' ? captureInfos[i].name : "Device " + std::to_string(i + 1);
      if (item.isDefault) {
        item.label += " (default)";
      }
      captureList.push_back(item);
    }
    if (hasCaptureSelection) {
      bool found = false;
      for (const ListedCaptureDevice& item : captureList) {
        if (ma_device_id_equal(&item.id, &selectedCaptureId) == MA_TRUE) {
          found = true;
          break;
        }
      }
      if (!found) {
        hasCaptureSelection = false;
      }
    }
  }

  int captureDeviceIndex() const {
    if (hasCaptureSelection) {
      for (int i = 0; i < static_cast<int>(captureList.size()); ++i) {
        if (ma_device_id_equal(&captureList[static_cast<size_t>(i)].id, &selectedCaptureId) == MA_TRUE) {
          return i;
        }
      }
    }
    for (int i = 0; i < static_cast<int>(captureList.size()); ++i) {
      if (captureList[static_cast<size_t>(i)].isDefault) {
        return i;
      }
    }
    return captureList.empty() ? -1 : 0;
  }

  bool openCapture(ma_device_type type, const char* failure) {
    ma_backend backends[] = {ma_backend_wasapi};
    ma_device_config config = ma_device_config_init(type);
    config.capture.format = ma_format_f32;
    config.capture.channels = 2;
    config.capture.pDeviceID = nullptr;
    if (type == ma_device_type_capture) {
      refreshCaptureDevices();
      if (hasCaptureSelection) {
        config.capture.pDeviceID = &selectedCaptureId;
      }
    }
    config.sampleRate = 48000;
    config.periodSizeInFrames = 512;
    config.performanceProfile = ma_performance_profile_low_latency;
    config.dataCallback = &AudioEngine::Impl::callback;
    config.pUserData = this;
    ma_result result = MA_ERROR;
    if (contextReady) {
      result = ma_device_init(&context, &config, &device);
    } else if (type == ma_device_type_loopback) {
      result = ma_device_init_ex(backends, 1, nullptr, &config, &device);
    } else {
      result = ma_device_init(nullptr, &config, &device);
    }
    if (result != MA_SUCCESS && type == ma_device_type_capture && config.capture.pDeviceID != nullptr) {
      hasCaptureSelection = false;
      config.capture.pDeviceID = nullptr;
      result = contextReady ? ma_device_init(&context, &config, &device) : ma_device_init(nullptr, &config, &device);
    }
    if (result != MA_SUCCESS) {
      error = describeFailure(failure, result);
      return false;
    }
    deviceOpen = true;
    sampleRate.store(static_cast<int>(device.sampleRate));
    if (type == ma_device_type_capture) {
      activeCaptureName = device.capture.name;
      if (!hasCaptureSelection) {
        selectedCaptureId = device.capture.id;
        hasCaptureSelection = true;
      }
    }
    const ma_result started = ma_device_start(&device);
    if (started != MA_SUCCESS) {
      error = describeFailure(failure, started);
      closeDevice();
      return false;
    }
    return true;
  }

  static void callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount) {
    auto* self = static_cast<Impl*>(device->pUserData);
    if (self != nullptr) {
      self->onAudio(output, input, frameCount);
    }
  }
};

AudioEngine::AudioEngine() : impl_(std::make_unique<Impl>()) {}

AudioEngine::~AudioEngine() {
  stop();
}

bool AudioEngine::start(AudioSource source) {
  if (source == AudioSource::File) {
    impl_->error = "Choose an audio file.";
    return false;
  }
  stop();
  impl_->error.clear();
  impl_->filePath.clear();
  impl_->toneSample = 0;
  impl_->clearRing();
  impl_->session.fetch_add(1, std::memory_order_relaxed);
  impl_->source = source;

  bool opened = false;
  if (source == AudioSource::System) {
    opened = impl_->openCapture(ma_device_type_loopback, "Could not capture system audio");
  } else if (source == AudioSource::Microphone) {
    opened = impl_->openCapture(ma_device_type_capture, "Could not open the microphone");
  } else {
    opened = impl_->openPlayback(AudioSource::Tone, 48000);
  }

  if (!opened && source != AudioSource::Tone) {
    const std::string kept = impl_->error;
    impl_->toneSample = 0;
    impl_->source = AudioSource::Tone;
    if (impl_->openPlayback(AudioSource::Tone, 48000)) {
      impl_->error = kept;
    }
    return false;
  }
  return opened;
}

bool AudioEngine::startFile(const std::wstring& path) {
  stop();
  impl_->error.clear();
  impl_->toneSample = 0;
  impl_->clearRing();
  impl_->session.fetch_add(1, std::memory_order_relaxed);

  ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 48000);
  const ma_result decoded = ma_decoder_init_file_w(path.c_str(), &decoderConfig, &impl_->decoder);
  if (decoded != MA_SUCCESS) {
    impl_->error = describeFailure("Could not open the file", decoded);
    impl_->filePath.clear();
    impl_->source = AudioSource::Tone;
    impl_->openPlayback(AudioSource::Tone, 48000);
    return false;
  }
  impl_->decoderOpen = true;
  ma_uint64 length = 0;
  ma_decoder_get_length_in_pcm_frames(&impl_->decoder, &length);
  impl_->fileLength = length;
  impl_->cursor.store(0, std::memory_order_relaxed);
  impl_->reachedEnd.store(false, std::memory_order_relaxed);
  impl_->playing.store(true, std::memory_order_relaxed);
  const int rate = static_cast<int>(impl_->decoder.outputSampleRate);
  if (!impl_->openPlayback(AudioSource::File, rate > 0 ? rate : 48000)) {
    const std::string kept = impl_->error;
    impl_->closeDevice();
    impl_->filePath.clear();
    impl_->openPlayback(AudioSource::Tone, 48000);
    impl_->error = kept;
    return false;
  }
  impl_->filePath = path;
  return true;
}

void AudioEngine::stop() {
  impl_->closeDevice();
}

AudioSource AudioEngine::source() const {
  return impl_->source;
}

int AudioEngine::sampleRate() const {
  return impl_->sampleRate.load();
}

const std::string& AudioEngine::lastError() const {
  return impl_->error;
}

const std::wstring& AudioEngine::filePath() const {
  return impl_->filePath;
}

uint64_t AudioEngine::framesWritten() const {
  return impl_->framesWritten.load();
}

uint64_t AudioEngine::session() const {
  return impl_->session.load();
}

void AudioEngine::readLatest(float* interleavedStereo, uint32_t frames) const {
  impl_->readLatest(interleavedStereo, frames);
}

bool AudioEngine::filePlaying() const {
  return impl_->source == AudioSource::File && impl_->playing.load(std::memory_order_relaxed);
}

double AudioEngine::filePositionSeconds() const {
  if (impl_->source != AudioSource::File) {
    return 0.0;
  }
  const double rate = impl_->sampleRate.load() > 0 ? static_cast<double>(impl_->sampleRate.load()) : 48000.0;
  return static_cast<double>(impl_->cursor.load(std::memory_order_relaxed)) / rate;
}

double AudioEngine::fileDurationSeconds() const {
  if (impl_->source != AudioSource::File || impl_->fileLength == 0) {
    return 0.0;
  }
  const double rate = impl_->sampleRate.load() > 0 ? static_cast<double>(impl_->sampleRate.load()) : 48000.0;
  return static_cast<double>(impl_->fileLength) / rate;
}

void AudioEngine::updateFilePlayback() {
  if (impl_->source == AudioSource::File && impl_->reachedEnd.load(std::memory_order_relaxed) &&
      impl_->playing.load(std::memory_order_relaxed) && impl_->deviceOpen) {
    ma_device_stop(&impl_->device);
    impl_->playing.store(false, std::memory_order_relaxed);
  }
}

void AudioEngine::setFilePlaying(bool wantPlaying) {
  if (impl_->source != AudioSource::File || !impl_->deviceOpen || !impl_->decoderOpen) {
    return;
  }
  if (wantPlaying) {
    const bool atEnd = impl_->fileLength > 0 && impl_->cursor.load(std::memory_order_relaxed) >= impl_->fileLength;
    if (atEnd || impl_->reachedEnd.load(std::memory_order_relaxed)) {
      seekFileSeconds(0.0);
    }
    if (!impl_->playing.load(std::memory_order_relaxed)) {
      impl_->reachedEnd.store(false, std::memory_order_relaxed);
      impl_->playing.store(true, std::memory_order_relaxed);
      if (ma_device_start(&impl_->device) != MA_SUCCESS) {
        impl_->playing.store(false, std::memory_order_relaxed);
        impl_->error = "Could not resume the file.";
      }
    }
    return;
  }
  if (impl_->playing.load(std::memory_order_relaxed)) {
    ma_device_stop(&impl_->device);
    impl_->playing.store(false, std::memory_order_relaxed);
  }
}

void AudioEngine::seekFileSeconds(double seconds) {
  if (impl_->source != AudioSource::File || !impl_->decoderOpen) {
    return;
  }
  const double rate = impl_->sampleRate.load() > 0 ? static_cast<double>(impl_->sampleRate.load()) : 48000.0;
  uint64_t frame = seconds <= 0.0 ? 0 : static_cast<uint64_t>(seconds * rate + 0.5);
  if (impl_->fileLength > 0 && frame > impl_->fileLength) {
    frame = impl_->fileLength;
  }
  const bool atEnd = impl_->fileLength > 0 && frame >= impl_->fileLength;
  const bool wasPlaying = impl_->playing.load(std::memory_order_relaxed);
  {
    std::lock_guard lock(impl_->decoderMutex);
    ma_decoder_seek_to_pcm_frame(&impl_->decoder, frame);
    impl_->cursor.store(frame, std::memory_order_relaxed);
    impl_->reachedEnd.store(atEnd, std::memory_order_relaxed);
    impl_->clearRing();
    if (!wasPlaying && !atEnd) {
      impl_->previewAt(frame);
    }
  }
  if (atEnd && wasPlaying && impl_->deviceOpen) {
    ma_device_stop(&impl_->device);
    impl_->playing.store(false, std::memory_order_relaxed);
  }
}

void AudioEngine::refreshCaptureDevices() {
  impl_->refreshCaptureDevices();
}

std::vector<std::string> AudioEngine::captureDeviceNames() const {
  std::vector<std::string> names;
  names.reserve(impl_->captureList.size());
  for (const auto& device : impl_->captureList) {
    names.push_back(device.label);
  }
  return names;
}

int AudioEngine::captureDeviceIndex() const {
  return impl_->captureDeviceIndex();
}

std::string AudioEngine::captureDeviceName() const {
  const int index = impl_->captureDeviceIndex();
  if (index >= 0 && index < static_cast<int>(impl_->captureList.size())) {
    return impl_->captureList[static_cast<size_t>(index)].label;
  }
  if (!impl_->activeCaptureName.empty()) {
    return impl_->activeCaptureName;
  }
  return "Default";
}

bool AudioEngine::setCaptureDevice(int index) {
  impl_->refreshCaptureDevices();
  if (index < 0 || index >= static_cast<int>(impl_->captureList.size())) {
    return false;
  }
  impl_->selectedCaptureId = impl_->captureList[static_cast<size_t>(index)].id;
  impl_->hasCaptureSelection = true;
  if (impl_->source == AudioSource::Microphone) {
    return start(AudioSource::Microphone);
  }
  return true;
}
