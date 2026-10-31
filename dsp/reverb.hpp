// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Chris Hoge
// Design after Sean Costello's reverbsc; no reverbsc code. See NOTICE.md.

#pragma once

#include "delayline.hpp"
#include "filters.hpp"
#include "utility.hpp"

/**
 * 8-line feedback delay network reverb with slowly modulated delay lengths.
 *
 * Structure inspired by Sean Costello's reverbsc (as used by Audrey II via
 * daisysp::ReverbSc): same line count, base delay times and modulation rates,
 * one-pole damping in each line. Mixing is a Householder reflection rather than
 * reverbsc's, and modulation is a sine LFO per line rather than random jitter.
 *
 * Output is fully wet. External SDRAM buffer of kMemorySize floats must be
 * assigned via set_memory() before use.
 */
class FdnReverb
{
public:
  static constexpr int kLines = 8;
  static constexpr size_t kLineLength = 8192;
  static constexpr size_t kMemorySize = kLines * kLineLength;

  void set_memory(float *mem)
  {
    for (int i = 0; i < kLines; ++i)
      lines[i].set_memory(mem + i * kLineLength);
  }

  void init(float samplerate)
  {
    for (int i = 0; i < kLines; ++i)
    {
      base_delay[i] = kBaseDelayS[i] * samplerate;
      lfo_inc[i] = kLfoHz[i] / samplerate;
      lfo_phase[i] = i / static_cast<float>(kLines);
      damping[i].set_freq(kDampingHz, samplerate);
    }
    mod_depth = kModDepthS * samplerate;
    reset();
  }

  void reset()
  {
    for (int i = 0; i < kLines; ++i)
    {
      lines[i].clear();
      damping[i].reset();
    }
  }

  // feedback: 0..<1, decay per pass through a line
  void set_feedback(float feedback)
  {
    gain = feedback;
    // keep the wet level roughly constant as decay grows, so turning REVERB up
    // doesn't push the outer loop over its threshold by itself
    // (exponent fitted by measurement: damping and modulation make the energy
    // grow slower than the lossless 1 / (1 - g^2))
    out_scale = kOutputTrim * std::pow(1.f - feedback * feedback, 0.3f);
  }

  // in: dry stereo sample, replaced by the wet signal
  void process(float &l, float &r)
  {
    float y[kLines];
    float sum = 0.f;
    for (int i = 0; i < kLines; ++i)
    {
      lfo_phase[i] += lfo_inc[i];
      if (lfo_phase[i] >= 1.f)
        lfo_phase[i] -= 1.f;
      const float d = base_delay[i] + mod_depth * (1.f + fast_sine(lfo_phase[i]));
      y[i] = damping[i].process(lines[i].read_linear(d));
      sum += y[i];
    }

    // Householder feedback matrix: y - (2/N) * sum(y), lossless before `gain`
    const float reflect = sum * (2.f / kLines);
    float wet_l = 0.f, wet_r = 0.f;
    for (int i = 0; i < kLines; i += 2)
    {
      lines[i].write(gain * (y[i] - reflect) + l);
      lines[i + 1].write(gain * (y[i + 1] - reflect) + r);
      wet_l += y[i];
      wet_r += y[i + 1];
    }

    l = wet_l * out_scale;
    r = wet_r * out_scale;
  }

private:
  // reverbsc base delay times (originally given in samples at 44.1 kHz)
  static constexpr float kBaseDelayS[kLines] = {
      2473.f / 44100.f, 2767.f / 44100.f, 3217.f / 44100.f, 3557.f / 44100.f,
      3907.f / 44100.f, 4127.f / 44100.f, 2143.f / 44100.f, 1933.f / 44100.f};
  static constexpr float kLfoHz[kLines] = {3.1f, 3.5f, 1.11f, 3.973f, 2.341f, 1.897f, 0.891f, 3.221f};
  static constexpr float kModDepthS = 0.0005f; // +-0.5 ms around (base + 0.5 ms)
  static constexpr float kDampingHz = 12000.f; // as Audrey's SetLpFreq(12000)
  static constexpr float kOutputTrim = 0.75f;

  // sine of 2*pi*phase for phase in [0, 1), parabolic approximation
  static float fast_sine(float phase)
  {
    const float x = phase * 2.f - 1.f; // [-1, 1)
    const float p = 4.f * x * (1.f - std::fabs(x));
    return -(p * (0.775f + 0.225f * std::fabs(p)));
  }

  DelayLine<kLineLength> lines[kLines];
  Tone damping[kLines];
  float base_delay[kLines] = {};
  float lfo_inc[kLines] = {};
  float lfo_phase[kLines] = {};
  float mod_depth = 0.f;
  float gain = 0.f;
  float out_scale = 0.f;
};

// C++11 needs out-of-class definitions for odr-used static constexpr arrays
constexpr float FdnReverb::kBaseDelayS[FdnReverb::kLines];
constexpr float FdnReverb::kLfoHz[FdnReverb::kLines];
