#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>

// Power-of-two delay line backed by externally allocated (SDRAM) memory.
// Read semantics match daisysp::DelayLine: read(1.f) returns the most recently
// written sample, so reading before writing in a sample gives a 1-sample loop.
template <size_t N>
class DelayLine
{
  static_assert((N > 1) && ((N & (N - 1)) == 0), "N must be power of 2");

public:
  static constexpr size_t kSize = N;

  void set_memory(float *mem) { samples = mem; }

  void clear()
  {
    if (samples)
      std::fill(samples, samples + N, 0.f);
  }

  void write(float sample)
  {
    samples[pos & mask] = sample;
    ++pos;
  }

  float read_linear(float delay) const
  {
    const size_t d = static_cast<size_t>(delay);
    const float frac = delay - static_cast<float>(d);
    const float a = tap(d);
    return a + frac * (tap(d + 1) - a);
  }

  // 4-point Hermite interpolation, as in daisysp::DelayLine::ReadHermite
  float read_hermite(float delay) const
  {
    const size_t d = static_cast<size_t>(delay);
    const float f = delay - static_cast<float>(d);
    const float xm1 = tap(d - 1);
    const float x0 = tap(d);
    const float x1 = tap(d + 1);
    const float x2 = tap(d + 2);
    const float c = (x1 - xm1) * 0.5f;
    const float v = x0 - x1;
    const float w = c + v;
    const float a = w + v + (x2 - x0) * 0.5f;
    const float b_neg = w + a;
    return (((a * f) - b_neg) * f + c) * f + x0;
  }

private:
  float tap(size_t delay) const
  {
    // unsigned wraparound + mask keeps this valid before the line has filled
    return samples[(pos - delay) & mask];
  }

  static constexpr size_t mask = N - 1;
  float *samples = nullptr;
  size_t pos = 0;
};
