Each file in this folder is one entry in the mode list. Press Refresh next to Mode after you add or remove a file.

A built-in visualization is a .viz file. The first line names the renderer:

  builtin bars
  builtin ring
  builtin oscilloscope
  builtin spectrogram
  builtin terrain
  builtin particles
  builtin lissajous
  builtin kaleidoscope
  builtin nebula
  builtin skyline
  builtin radar
  builtin ripples
  builtin stereo
  builtin helix

The name shown in the list comes from the file name. 01-Bars.viz is shown as Bars. Delete the file to remove that mode. Copy it back to restore it.

A .frag file is a custom full-screen shader. It can use vUv, uTime, uResolution, uPalette, uBass, uMid, uTreble, uEnergy, bandAt, peakAt, waveAt, historyAt and paletteColor. Files that start with _ are ignored.
