# Audio Visualizer

Windows desktop app that shows the same sound in several ways: bars, ring, oscilloscope, spectrogram, 3D terrain, particles, Lissajous, and kaleidoscope.

The analysis keeps every bin of a 4096-sample FFT (up to half the sample rate, about 22 kHz) and also groups them into logarithmic bands so the bass stays visible.

## Requirements

- Windows 10 or later
- Visual Studio 2022 (or Build Tools) with the C++ workload
- CMake 3.24 or later
- Git, because the first configure step downloads GLFW, Dear ImGui, and miniaudio

## Build

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The executable is `build\Release\audio-visualizer.exe`.

## Use

The app starts with a test tone so the modes are visible without a microphone or music. The panel selects the source:

- **System**: whatever is playing on the PC (the default playback device)
- **Microphone**: a chosen input device
- **File**: wav, mp3, or flac. You can also drop a file onto the window. Play, pause, and the time slider control playback.
- **Test tone**: a moving bass, chord, and highs

Keys:

- `1` through `8`: mode
- `Space`: play or pause a file
- `Tab`: show or hide the panel
- `F11`: fullscreen
- `Esc`: leave fullscreen, or close the window

System capture does not include apps in exclusive mode. The microphone depends on the Windows privacy permission.
