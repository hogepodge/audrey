// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Chris Hoge
// Port of Audrey II's KarplusString (MIT, (c) 2026 Synthux Academy), itself a
// modified daisysp::KarplusString (MIT, (c) 2020 Electrosmith, Corp). See NOTICE.md.

#pragma once

#include "delayline.hpp"
#include "filters.hpp"
#include "utility.hpp"

/**
 * Karplus-Strong string resonator, ported from Audrey-II's infrasonic::KarplusString
 * (itself a modified daisysp::KarplusString, MIT, Electrosmith).
 *
 * The sub-11.7 Hz upsampling path is dropped: PITCH bottoms out above that.
 * External SDRAM buffer of kLength floats must be assigned via set_memory() before use.
 */
class KarplusString
{
public:
  static constexpr size_t kLength = 8192;

  void set_memory(float *mem) { line.set_memory(mem); }

  void init(float samplerate)
  {
    damping_filter.set_freq(8000.f, samplerate);
    reset();
  }

  void reset()
  {
    line.clear();
    damping_filter.reset();
    dc_blocker.reset();
  }

  // delay_samples: string period in samples (samplerate / freq)
  float process(float in, float delay_samples)
  {
    const float delay = clampf(delay_samples, 4.f, kLength - 4.f);
    float s = line.read_hermite(delay);
    s += in;
    s = clampf(s, -20.f, 20.f);
    s = dc_blocker.process(s);
    s *= 0.8f;
    s = damping_filter.process(s);
    line.write(s);
    return s;
  }

private:
  DelayLine<kLength> line;
  Tone damping_filter;
  DcBlock dc_blocker;
};
