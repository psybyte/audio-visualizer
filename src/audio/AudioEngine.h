#pragma once

#include "Types.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class AudioEngine {
 public:
  AudioEngine();
  ~AudioEngine();
  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;

  bool start(AudioSource source);
  bool startFile(const std::wstring& path);
  void stop();

  AudioSource source() const;
  int sampleRate() const;
  const std::string& lastError() const;
  const std::wstring& filePath() const;
  uint64_t framesWritten() const;
  uint64_t session() const;
  void readLatest(float* interleavedStereo, uint32_t frames) const;

  bool filePlaying() const;
  double filePositionSeconds() const;
  double fileDurationSeconds() const;
  void updateFilePlayback();
  void setFilePlaying(bool playing);
  void seekFileSeconds(double seconds);

  void refreshCaptureDevices();
  std::vector<std::string> captureDeviceNames() const;
  int captureDeviceIndex() const;
  std::string captureDeviceName() const;
  bool setCaptureDevice(int index);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
