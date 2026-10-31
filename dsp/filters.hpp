#pragma once

#include <cmath>

// Stereo 2nd order biquad, transposed direct form 2.
// Coefficients follow Audrey-II's BiquadSection (bilinear, prewarped K = tan(pi*fc/fs)).
class StereoBiquad
{
public:
  void set_lowpass(float cutoff_hz, float q, float samplerate)
  {
    const float K = std::tan(static_cast<float>(M_PI) * cutoff_hz / samplerate);
    const float Ksq = K * K;
    const float norm = 1.f / (1.f + K / q + Ksq);
    b0 = Ksq * norm;
    b1 = 2.f * b0;
    b2 = b0;
    a1 = 2.f * (Ksq - 1.f) * norm;
    a2 = (1.f - K / q + Ksq) * norm;
  }

  void set_highpass(float cutoff_hz, float q, float samplerate)
  {
    const float K = std::tan(static_cast<float>(M_PI) * cutoff_hz / samplerate);
    const float Ksq = K * K;
    const float norm = 1.f / (1.f + K / q + Ksq);
    b0 = norm;
    b1 = -2.f * b0;
    b2 = b0;
    a1 = 2.f * (Ksq - 1.f) * norm;
    a2 = (1.f - K / q + Ksq) * norm;
  }

  void reset()
  {
    s1[0] = s1[1] = 0.f;
    s2[0] = s2[1] = 0.f;
  }

  void process(float &l, float &r)
  {
    l = process_channel(l, 0);
    r = process_channel(r, 1);
  }

private:
  float process_channel(float x, int ch)
  {
    const float y = b0 * x + s1[ch];
    s1[ch] = s2[ch] + b1 * x - a1 * y;
    s2[ch] = b2 * x - a2 * y;
    return y;
  }

  float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
  float s1[2] = {0.f, 0.f};
  float s2[2] = {0.f, 0.f};
};

// One-pole lowpass, as daisysp::Tone
class Tone
{
public:
  void set_freq(float freq_hz, float samplerate)
  {
    const float b = 2.f - std::cos(2.f * static_cast<float>(M_PI) * freq_hz / samplerate);
    c2 = b - std::sqrt(b * b - 1.f);
    c1 = 1.f - c2;
  }

  void reset() { prev = 0.f; }

  float process(float x)
  {
    prev = c1 * x + c2 * prev;
    return prev;
  }

private:
  float c1 = 0.5f, c2 = 0.5f;
  float prev = 0.f;
};

// DC blocker, structured as daisysp::DcBlock. daisysp uses a 0.99 pole (~76 Hz at
// 48 kHz), which starves the loop at PITCH's bottom notes; 0.995 (~38 Hz) keeps
// the lower-left of the pad slow to build but not silent.
class DcBlock
{
public:
  void reset() { x1 = y1 = 0.f; }

  float process(float x)
  {
    const float y = x - x1 + kGain * y1;
    x1 = x;
    y1 = y;
    return y;
  }

private:
  static constexpr float kGain = 0.995f;
  float x1 = 0.f, y1 = 0.f;
};
