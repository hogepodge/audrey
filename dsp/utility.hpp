// SPDX-License-Identifier: MIT AND BSD-3-Clause
// Copyright (c) 2026 Chris Hoge
// The helpers up to fast_tanh come from the KORG logue SDK pluck example
// (BSD-3-Clause, (c) 2018 KORG INC.). ftension is from Audrey II (MIT,
// (c) 2026 Synthux Academy); soft_clip and Overdrive follow DaisySP (MIT,
// (c) 2020 Electrosmith, Corp). See NOTICE.md.

#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>

constexpr float A440Frequency = 440.0f;
constexpr int A440NoteNumber = 69;

inline float note_to_hz(int note)
{
  return A440Frequency *
         std::pow(2.0f, (static_cast<float>(note) - A440NoteNumber) / 12.0f);
}

// [0,1] -> [20, 20000] Hz
inline float norm_to_freq(float x)
{
  return 2.f * powf(10.f, 3.f * x + 1.f);
}

inline float lerpf(float x0, float x1, float frac)
{
  return x0 + frac * (x1 - x0);
}

inline float clampf(float x, float lo, float hi)
{
  return std::min(std::max(x, lo), hi);
}

// https://www.desmos.com/calculator/merzdbysff
inline float fast_tanh(float x)
{
  return x * (27.f + x * x) / (27.f + 9.f * x * x);
}

inline float db_to_lin(float db)
{
  return std::pow(10.f, db * 0.05f);
}

// [0,1] -> [lo, hi], exponential
inline float norm_to_exp(float x, float lo, float hi)
{
  return lo * std::pow(hi / lo, x);
}

// Per-sample one-pole smoothing coefficient for time constant tau_s
inline float onepole_coef(float tau_s, float samplerate)
{
  if (tau_s <= 0.f)
    return 1.f;
  return std::min(1.f / (tau_s * samplerate), 1.f);
}

// One-pole smoothing coefficient applied once per block of `frames` samples
inline float block_coef(float tau_s, float samplerate, uint32_t frames)
{
  if (tau_s <= 0.f)
    return 1.f;
  return 1.f - std::exp(-static_cast<float>(frames) / (tau_s * samplerate));
}

// Audrey II's ftension: bends [0,1] into a curve; negative factor rises fast then levels off
inline float ftension(float x, float factor)
{
  if (factor == 0.f)
    return x;
  return std::expm1(x * factor) / std::expm1(factor);
}

// daisysp::SoftClip
inline float soft_clip(float x)
{
  if (x < -3.f)
    return -1.f;
  if (x > 3.f)
    return 1.f;
  return fast_tanh(x);
}

// daisysp::Overdrive (stateless, so one instance can serve both channels)
class Overdrive
{
public:
  void set_drive(float drive)
  {
    const float d = 2.f * clampf(drive, 0.f, 1.f);
    const float d2 = d * d;
    const float pre_a = d * 0.5f;
    const float pre_b = d2 * d2 * d * 24.f;
    pre_gain = pre_a + (pre_b - pre_a) * d2;
    const float squashed = d * (2.f - d);
    post_gain = 1.f / soft_clip(0.33f + squashed * (pre_gain - 0.33f));
  }

  // Clip shape: 0 = round symmetric soft clip (daisysp), 1 = biased hard clip
  // (lopsided, rich in even harmonics). The DC the bias creates is removed by the
  // string's DC blocker inside the loop and by the HPF before the output.
  void set_character(float character)
  {
    hardness = clampf(character, 0.f, 1.f);
    bias = kMaxBias * hardness;
    // measure slope at 0 so callers can compensate loop gain for the shape change
    const float eps = 1e-3f;
    shape_slope = (shape(eps) - shape(-eps)) / (2.f * eps);
  }

  float process(float x) const { return shape(pre_gain * x) * post_gain; }

  // Gain for signals well below the clipping knee
  float small_signal_gain() const { return pre_gain * post_gain * shape_slope; }

private:
  static constexpr float kMaxBias = 0.6f;

  float shape(float u) const
  {
    const float soft = soft_clip(u + bias) - soft_clip(bias);
    const float hard = clampf(u + bias, -1.f, 1.f) - bias;
    return lerpf(soft, hard, hardness);
  }

  float pre_gain = 1.f;
  float post_gain = 1.f;
  float hardness = 0.f;
  float bias = 0.f;
  float shape_slope = 1.f;
};

// xorshift32 white noise in [-1, 1)
class WhiteNoise
{
public:
  float process()
  {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return static_cast<int32_t>(state) * (1.f / 2147483648.f);
  }

  void seed(uint32_t s) { state = s ? s : 0x1234567U; }

private:
  uint32_t state = 0x1234567U;
};

// Smooth random drift in [-1, 1], in time with the music: holds each random
// target for 1, 2 or 4 beats and glides toward it, so blooms and fades land on
// the beat grid. Advanced once per audio block by the beats that began in it.
class Wander
{
public:
  void seed(uint32_t s) { rng.seed(s); }

  // beats: beat boundaries crossed this block; beat_s: current length of a beat
  float process_block(uint32_t frames, float samplerate, uint32_t beats, float beat_s)
  {
    countdown -= static_cast<int32_t>(beats);
    if (countdown <= 0 && beats > 0)
    {
      target = rng.process();
      const int choice = std::min(2, static_cast<int>(1.5f * (rng.process() + 1.f)));
      hold_beats = kHoldBeats[choice];
      countdown = hold_beats;
    }
    // glide over about half the hold, at the current tempo
    value += (target - value) * block_coef(0.5f * hold_beats * beat_s, samplerate, frames);
    return value;
  }

private:
  static constexpr int kHoldBeats[3] = {1, 2, 4};
  WhiteNoise rng;
  int32_t countdown = 0;
  int32_t hold_beats = 1;
  float target = 0.f;
  float value = 0.f;
};

// C++11 needs an out-of-class definition for an odr-used static constexpr array
constexpr int Wander::kHoldBeats[3];
