#include "analysis/Fft.h"

#include <cmath>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979323846f;

void bitReverse(float* real, float* imag, int size) {
  int j = 0;
  for (int i = 1; i < size; ++i) {
    int bit = size >> 1;
    for (; (j & bit) != 0; bit >>= 1) {
      j ^= bit;
    }
    j ^= bit;
    if (i < j) {
      std::swap(real[i], real[j]);
      std::swap(imag[i], imag[j]);
    }
  }
}

int peakBin(const float* real, const float* imag, int size) {
  int peak = 1;
  float best = 0.f;
  for (int i = 1; i < size / 2; ++i) {
    const float mag = real[i] * real[i] + imag[i] * imag[i];
    if (mag > best) {
      best = mag;
      peak = i;
    }
  }
  return peak;
}

}  // namespace

void fftRadix2(float* real, float* imag, int size) {
  bitReverse(real, imag, size);
  for (int length = 2; length <= size; length <<= 1) {
    const float angle = -2.f * kPi / static_cast<float>(length);
    const float wlenReal = std::cos(angle);
    const float wlenImag = std::sin(angle);
    for (int start = 0; start < size; start += length) {
      float wReal = 1.f;
      float wImag = 0.f;
      const int half = length / 2;
      for (int k = 0; k < half; ++k) {
        const int even = start + k;
        const int odd = even + half;
        const float oddReal = wReal * real[odd] - wImag * imag[odd];
        const float oddImag = wReal * imag[odd] + wImag * real[odd];
        real[odd] = real[even] - oddReal;
        imag[odd] = imag[even] - oddImag;
        real[even] += oddReal;
        imag[even] += oddImag;
        const float nextReal = wReal * wlenReal - wImag * wlenImag;
        wImag = wReal * wlenImag + wImag * wlenReal;
        wReal = nextReal;
      }
    }
  }
}

bool fftSelfTest(std::string& error) {
  constexpr int kSize = 4096;
  constexpr int kBins[] = {48, 86, 700};
  for (int bin : kBins) {
    std::vector<float> real(kSize, 0.f);
    std::vector<float> imag(kSize, 0.f);
    for (int n = 0; n < kSize; ++n) {
      const float hann = 0.5f * (1.f - std::cos(2.f * kPi * static_cast<float>(n) / static_cast<float>(kSize - 1)));
      real[static_cast<size_t>(n)] = hann * std::sin(2.f * kPi * static_cast<float>(bin) * static_cast<float>(n) / static_cast<float>(kSize));
    }
    fftRadix2(real.data(), imag.data(), kSize);
    const int peak = peakBin(real.data(), imag.data(), kSize);
    if (peak != bin) {
      std::ostringstream stream;
      stream << "The FFT placed a sine from bin " << bin << " into bin " << peak << ".";
      error = stream.str();
      return false;
    }
  }
  return true;
}
