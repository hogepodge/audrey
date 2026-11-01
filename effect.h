/*
 *  File: effect.h
 *
 *  AUDREY: resonating feedback horror synth for NTS-3 kaoss pad kit.
 *  Based on Audrey II by Nick Donaldson / Synthux Academy (MIT).
 *
 *  SPDX-License-Identifier: MIT
 *  Copyright (c) 2026 Chris Hoge. See NOTICE.md for third-party credits.
 *
 *  input --x INPUT, level-capped (and lifts the loop gain while it arrives)
 *  noise (-90 dB) --+  |
 *                   v  v
 *    +------------> + --> KS string -> drive -> LPF -> HPF -> reverb --+--> soft clip -> out
 *    |                                                                 |
 *    +--- body delay <-- x loop gain <---------------------------------+
 *
 *  Touching the pad ignites the loop: HUNGER sweeps up from zero in 30 ms, and
 *  when the touch ends it slides back down to zero over RELEASE.
 */

#include <cmath>

#include "processor.h"
#include "unit_genericfx.h"

#include "dsp/utility.hpp"
#include "dsp/delayline.hpp"
#include "dsp/filters.hpp"
#include "dsp/ks_string.hpp"
#include "dsp/reverb.hpp"

class Effect : public Processor
{
public:
  static constexpr size_t kBodyLength = 32768; // 8 periods of E0, 680 ms at 48 kHz

  uint32_t getBufferSize() const override final
  {
    return 2 * KarplusString::kLength + 2 * kBodyLength + FdnReverb::kMemorySize;
  }

  enum
  {
    PITCH = 0U,
    HUNGER,
    INPUT,
    BODY,
    DRIVE,
    RATE,
    REVERB,
    RELEASE,
    NUM_PARAMS
  };

  // Note: Make sure that default param values correspond to declarations in header.c
  struct Params
  {
    float pitch;   // MIDI note number
    float hunger;  // 0..1
    float input;   // 0..1
    float body;    // 0..1
    float drive;   // 0..1, clip shape
    float rate;    // 0..1, drift tempo division
    float reverb;  // 0..1
    float release; // 0..1

    void reset()
    {
      pitch = 40.f;
      hunger = 0.f;
      input = param_10bit_to_f32(512);
      body = param_10bit_to_f32(725);
      drive = 0.f;
      rate = param_10bit_to_f32(512);
      reverb = param_10bit_to_f32(600);
      release = param_10bit_to_f32(795);
    }

    Params() { reset(); }
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case PITCH:
      // same scale as the stock OSC effects: 192 = C1, 768 = C7
      params_.pitch = value / 8.f;
      break;
    case HUNGER:
      params_.hunger = param_10bit_to_f32(value);
      break;
    case INPUT:
      params_.input = param_10bit_to_f32(value);
      break;
    case BODY:
      params_.body = param_10bit_to_f32(value);
      break;
    case DRIVE:
      params_.drive = param_10bit_to_f32(value);
      break;
    case RATE:
      params_.rate = param_10bit_to_f32(value);
      break;
    case REVERB:
      params_.reverb = param_10bit_to_f32(value);
      break;
    case RELEASE:
      params_.release = param_10bit_to_f32(value);
      break;
    default:
      break;
    }
  }

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    (void)index;
    (void)value;
    return nullptr;
  }

  // life-cycle methods
  void init(float *allocated_buffer) override final
  {
    float *mem = allocated_buffer;
    for (int i = 0; i < 2; ++i)
    {
      strings_[i].set_memory(mem);
      mem += KarplusString::kLength;
      body_[i].set_memory(mem);
      mem += kBodyLength;
    }
    reverb_.set_memory(mem);

    params_.reset();
    for (int i = 0; i < 2; ++i)
      strings_[i].init(getSampleRate());
    drive_.set_drive(kAudreyDrive);
    reverb_.init(getSampleRate());
    for (int i = 0; i < 3; ++i)
      wander_[i] = Wander();
    wander_[0].seed(0xA0D3E1U);
    wander_[1].seed(0x5EED2U);
    wander_[2].seed(0xB12EA7U);

    // start smoothed state at targets so loading the unit doesn't glide
    pitch_ = params_.pitch;
    string_delay_ = pitch_to_delay(pitch_);
    lpf_hz_ = hunger_to_lpf_hz(params_.hunger);
    hpf_.set_highpass(kLowCutHz, kFilterQ, getSampleRate());
    fwd_hpf_.set_highpass(kLowCutHz, kFilterQ, getSampleRate());
    body_pos_ = params_.body;
    hunger_ = params_.hunger;
    reverb_pos_ = params_.reverb;
    fb_gain_ = 0.f;
    excite_gain_ = 0.f;
    gate_ = 0.f;
    touching_ = false;
    tick_count_ = 0;
    tick_phase_ = 0.f;
    samples_since_ext_tick_ = 0xFFFFFFU;

    clear_state();
  }

  void teardown() override final {}

  void reset() override final { clear_state(); }

  void resume() override final { clear_state(); }

  // audio processing callbacks
  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    const Params p = params_;
    const float sr = getSampleRate();

    // ---- block-rate control smoothing ----
    pitch_ += (p.pitch - pitch_) * block_coef(kPitchLag, sr, frames);
    hunger_ += (p.hunger - hunger_) * block_coef(kGainLag, sr, frames);
    drive_.set_character(p.drive);

    // INPUT feeds the input into the beast's string, as AudreyTouch does, so it
    // circulates through the string and reverb and comes out as tones. Its
    // level there is capped: a loud input saturates the shared drive and takes
    // over. Your dry sound is left to a parallel chain on the NTS-3.
    float in_peak = 0.f;
    for (uint32_t i = 0; i < frames * 2; ++i)
      in_peak = std::max(in_peak, std::fabs(in[i]));
    input_env_ += (in_peak - input_env_) * block_coef(in_peak > input_env_ ? kInputAttack : kInputRelease, sr, frames);
    // a slow envelope for the cap: it only catches up with sustained input
    sustain_env_ += (in_peak - sustain_env_) * block_coef(in_peak > sustain_env_ ? kSustainAttack : kSustainRelease, sr, frames);
    // Feeding the beast makes it hungrier: while input arrives the loop gain is
    // lifted to just under the threshold, so what you feed in keeps
    // circulating. It holds for a while after the input stops, then lets go,
    // whatever the input's level; it is full from halfway up INPUT.
    feed_age_ = input_env_ > kFeedFull ? 0.f : std::min(feed_age_ + frames / sr, 60.f);
    const float feed_lift = std::min(1.f, 2.f * p.input) * clampf(1.f - (feed_age_ - kFeedHold) / kFeedRelease, 0.f, 1.f);


    // The gate slides linearly: a quick attack on touch, and on release it drags
    // HUNGER back down to zero over RELEASE, so the loop sinks through its
    // threshold and darkens as it goes, whatever height Y was at.
    const float gate_rate = touching_ ? 1.f / kAttackTime : -1.f / release_to_seconds(p.release);
    const float gate_end = clampf(gate_ + gate_rate * frames / sr, 0.f, 1.f);
    const float hunger_eff = hunger_ * gate_end;

    lpf_hz_ += (hunger_to_lpf_hz(hunger_eff) - lpf_hz_) * block_coef(kFilterLag, sr, frames);
    lpf_.set_lowpass(lpf_hz_, kFilterQ, sr);
    fwd_lpf_.set_lowpass(lpf_hz_, kFilterQ, sr);

    // REVERB: mix rises linearly, decay on Audrey's anti-exponential curve
    reverb_pos_ += (p.reverb - reverb_pos_) * block_coef(kFilterLag, sr, frames);
    const float reverb_mix = reverb_pos_;
    const float reverb_fb = lerpf(kReverbFbMin, kReverbFbMax, ftension(reverb_pos_, -3.f));
    reverb_.set_feedback(reverb_fb);

    // string period ramps linearly across the block
    const float string_delay_target = pitch_to_delay(pitch_);
    const float string_delay_inc = (string_delay_target - string_delay_) / frames;

    // Beat clock: follow the NTS-3's 16th-note tick counter when ticks arrive
    // (a beat starts every 4th tick), otherwise count 16ths from the tempo
    const float tick_s = 15.f / tempo_bpm_;
    const uint32_t ext_ticks = ext_ticks_;
    const bool ext_ticked = ext_ticks != ext_ticks_seen_;
    ext_ticks_seen_ = ext_ticks;
    samples_since_ext_tick_ = ext_ticked ? 0 : std::min<uint32_t>(samples_since_ext_tick_ + frames, 0xFFFFFFU);
    const uint32_t prev_tick_count = tick_count_;
    if (ext_ticked)
    {
      tick_count_ = ext_tick_counter_;
      tick_phase_ = 0.f;
    }
    else if (samples_since_ext_tick_ > 2.f * tick_s * sr)
    {
      tick_phase_ += frames;
      for (const float tick_samples = tick_s * sr; tick_phase_ >= tick_samples; tick_phase_ -= tick_samples)
        ++tick_count_;
    }
    // RATE picks the drift's step on that grid, 1/16 to 4 bars
    const uint32_t step_ticks = rate_to_ticks(p.rate);
    const uint32_t steps = tick_count_ / step_ticks - prev_tick_count / step_ticks;
    const float step_s = step_ticks * tick_s;

    // body delays drift on the tempo grid (independently per side) so resonances
    // slide in and out of alignment: the loop swells, sags and comes back
    const float wander_l = 1.f + kWanderDepth * wander_[0].process_block(frames, sr, steps, step_s);
    const float wander_r = 1.f + kWanderDepth * wander_[1].process_block(frames, sr, steps, step_s);
    // and the loop gain drifts a few dB, dipping the lower half of HUNGER below
    // its threshold now and then so the sound fades out and blooms back
    const float wander_db = kWanderGainDb * wander_[2].process_block(frames, sr, steps, step_s);

    // loop gain ramps linearly across the block; the last stretch of the gate
    // fades it fully out so an untouched loop is open
    const float fb_gain_end = hunger_to_loop_gain(hunger_eff, wander_db, feed_lift * gate_end) * std::min(1.f, gate_end * kGateFadeOut);
    const float fb_gain_inc = (fb_gain_end - fb_gain_) / frames;
    gate_ = gate_end;

    // ---- per-sample smoothing targets ----
    // The cap limits the input's sustained level, not its peaks: a steady
    // input that holds the drive in saturation takes over, but a hit only
    // saturates it for a moment and the beast recovers before the next one.
    // So the gain follows a slow envelope (quiet input is boosted up to
    // kExciteMaxGain), and hits pass up to kTransientHeadroom over the cap
    // before a soft clip catches them. INPUT's lower half raises the cap to
    // the most the beast stays on top of; the upper half keeps raising it,
    // bringing the input forward until it takes over at the top.
    const float excite_cap = p.input < 0.5f ? kExciteCap * std::max(2.f * p.input, 0.001f)
                                            : norm_to_exp(2.f * p.input - 1.f, kExciteCap, kExciteCapMax);
    const float excite_gain_target = std::min(kExciteMaxGain, excite_cap / (sustain_env_ + 1e-6f));
    const float excite_ceiling = excite_cap * kTransientHeadroom;
    // Your input is also heard directly, through a copy of the loop's filters,
    // joining at the output so it never goes around the loop (anything loud in
    // the loop takes it over). Faint at the bottom of HUNGER, it rises with
    // HUNGER to sit just above the beast's chaos at the top. INPUT scales it.
    const float fwd_gain_target = std::min(1.f, 2.f * p.input) * db_to_lin(lerpf(kFwdLowDb, kFwdHighDb, hunger_eff));
    const float gain_c = onepole_coef(kGainLag, sr);
    const float body_c = onepole_coef(kBodyLag, sr);

    for (const float *out_end = out + frames * 2; out != out_end; in += 2, out += 2)
    {
      fb_gain_ += fb_gain_inc;
      excite_gain_ += (excite_gain_target - excite_gain_) * gain_c;
      fwd_gain_ += (fwd_gain_target - fwd_gain_) * gain_c;
      body_pos_ += (p.body - body_pos_) * body_c;
      string_delay_ += string_delay_inc;
      const float body_delay = body_to_delay(body_pos_, string_delay_);

      const float noise = noise_.process() * kNoiseAmp;
      // ---> Feedback loop: body delay + noise + capped input into the string
      const float el = excite_ceiling * soft_clip(in[0] * excite_gain_ / excite_ceiling);
      const float er = excite_ceiling * soft_clip(in[1] * excite_gain_ / excite_ceiling);
      float l = body_[0].read_linear(clampf(body_delay * wander_l, 1.f, kBodyLength - 2.f)) + noise + el;
      float r = body_[1].read_linear(clampf(body_delay * wander_r - 4.f, 1.f, kBodyLength - 2.f)) + noise + er;

      l = strings_[0].process(l, string_delay_);
      r = strings_[1].process(r, string_delay_);

      l = drive_.process(l);
      r = drive_.process(r);

      lpf_.process(l, r);
      hpf_.process(l, r);

      // ---> Reverb, crossfaded in the loop as Audrey
      float wl = l, wr = r;
      reverb_.process(wl, wr);
      l += (wl - l) * reverb_mix;
      r += (wr - r) * reverb_mix;

      body_[0].write(l * fb_gain_);
      body_[1].write(r * fb_gain_);

      // ---> Your input, heard directly
      float fl = in[0] * fwd_gain_, fr = in[1] * fwd_gain_;
      fwd_lpf_.process(fl, fr);
      fwd_hpf_.process(fl, fr);

      out[0] = soft_clip((l + fl) * kOutputGain) * kOutputLevel;
      out[1] = soft_clip((r + fr) * kOutputGain) * kOutputLevel;
    }

    string_delay_ = string_delay_target;
  }

  void setTempo(float bpm) override final { tempo_bpm_ = clampf(bpm, 10.f, 600.f); }

  // may arrive outside render; only records the counter for process() to read
  void tempo4ppqnTick(uint32_t counter) override final
  {
    ext_tick_counter_ = counter;
    ext_ticks_ = ext_ticks_ + 1;
  }

  inline void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) override final
  {
    // x/y already reach PITCH/HUNGER through the parameter mappings
    (void)id;
    (void)x;
    (void)y;

    switch (phase)
    {
    case k_unit_touch_phase_began:
    case k_unit_touch_phase_moved:
    case k_unit_touch_phase_stationary:
      touching_ = true;
      break;
    case k_unit_touch_phase_ended:
    case k_unit_touch_phase_cancelled:
      touching_ = false;
      break;
    default:
      break;
    }
  }

private:
  // Tuning constants. Gain/curve ranges are first guesses to be tuned by ear.
  static constexpr float kNoiseAmp = 3.1623e-5f; // -90 dBFS, as Audrey
  // HUNGER curve knots (see hunger_to_db). The KS string adds ~+14 dB at its
  // resonant peaks, so the loop wakes up around -9 dB; wander keeps it breathing
  // up to ~6 dB above that.
  static constexpr float kHungerMinDb = -14.f;
  static constexpr float kHungerWakeH = 0.1f;
  static constexpr float kHungerWakeDb = -9.f;
  static constexpr float kHungerBreatheH = 0.55f;
  static constexpr float kHungerBreatheDb = -2.f;
  static constexpr float kHungerMaxDb = 24.f;
  static constexpr float kAudreyDrive = 0.4f; // fixed daisysp Overdrive amount, as Audrey
  static constexpr float kLpfMinHz = 800.f;
  static constexpr float kLpfMaxHz = 18000.f;
  static constexpr float kLowCutHz = 250.f; // in-loop HPF, as Audrey's default; use a stock HPF after for more
  static constexpr float kFilterQ = 0.9f;
  static constexpr float kReverbThresholdDb = 6.f; // measured threshold shift at full REVERB
  static constexpr float kReverbFbMin = 0.2f; // Audrey's reverb feedback range
  static constexpr float kReverbFbMax = 0.97f; // (Audrey goes to 1.0; capped to keep a finite tail)
  static constexpr float kWanderDepth = 0.12f; // body delay drift, fraction of its length
  static constexpr float kWanderGainDb = 6.f;  // loop gain drift, +- dB
  static constexpr float kBodyTrack = 0.21f; // chosen by ear in websim (TRACK 215)
  static constexpr float kBodyMinS = 0.001f;
  static constexpr float kBodyMaxS = 0.1f;
  static constexpr float kBodyMinRatio = 0.5f; // string periods
  static constexpr float kBodyMaxRatio = 8.f;
  static constexpr float kReleaseMinS = 0.1f;
  static constexpr float kReleaseMaxS = 20.f;
  static constexpr float kExciteCap = 0.03f;       // peak input into the string at INPUT 512: the beast stays on top
  static constexpr float kExciteCapMax = 0.12f;    // ... at full INPUT: the input takes over (the tipping point is ~0.06-0.09)
  static constexpr float kFeedSustainDb = -10.5f;  // loop gain while fed: just under the ~-9 dB threshold
  static constexpr float kFeedFull = 0.01f;        // input above -40 dBFS counts as feeding
  static constexpr float kFeedHold = 2.f;          // the lift holds this long after the input stops,
  static constexpr float kFeedRelease = 2.f;       // then fades out over this long
  static constexpr float kExciteMaxGain = 4.f;   // boost for quiet input, +12 dB
  static constexpr float kInputAttack = 0.01f;  // input envelope, for the cap and the lift
  static constexpr float kInputRelease = 0.15f;
  static constexpr float kFwdLowDb = -28.4f;  // direct input at HUNGER 0 (FWD 50 in the tuning build)
  static constexpr float kFwdHighDb = -6.6f;  // ... at full HUNGER (FWD 175): just above the beast
  static constexpr float kSustainAttack = 0.3f;  // the cap's envelope: slow enough to let hits through
  static constexpr float kSustainRelease = 1.f;
  static constexpr float kTransientHeadroom = 8.f; // hits may peak this far over the cap (+18 dB)
  // the beast's level is as before (-6 dB, LEVEL's old default), with 6 dB
  // more headroom before the output soft clip so the input can punch through
  static constexpr float kOutputGain = 0.25f;
  static constexpr float kOutputLevel = 1.f;
  static constexpr float kRateCurve = 1.485f; // 7 * 0.5^1.485 = 2.5: the middle of the beat zone
  static constexpr float kAttackTime = 0.03f;
  static constexpr float kGateFadeOut = 10.f; // loop gain fades out over the last 10% of the gate
  static constexpr float kPitchLag = 0.04f; // short glide; Audrey's 0.2 s felt laggy on the pad
  static constexpr float kFilterLag = 0.02f;
  static constexpr float kGainLag = 0.02f;
  static constexpr float kBodyLag = 0.2f;

  float pitch_to_delay(float note) const
  {
    const float hz = A440Frequency * std::pow(2.f, (note - A440NoteNumber) / 12.f);
    return getSampleRate() / hz;
  }

  // Compensates for the drive's small-signal gain and the reverb's phase smearing
  // so the self-oscillation threshold sits at the same HUNGER position
  // regardless of DRIVE and REVERB. `wander_db` is the gain drift. `lift`
  // (0..1) raises the gain toward just under the threshold, as a floor the
  // drift can't pull it below.
  float hunger_to_loop_gain(float h, float wander_db, float lift) const
  {
    const float base = hunger_to_db(h) + wander_db;
    const float db = lerpf(base, std::max(base, kFeedSustainDb), lift) + kReverbThresholdDb * reverb_pos_;
    return db_to_lin(db) / drive_.small_signal_gain();
  }

  // HUNGER -> loop gain in dB, relative to the drive's small-signal gain.
  // Piecewise: silent below the wake-up point (~10% of Y), then a long, shallow
  // stretch just above it where the wandering loop fades out and blooms back,
  // then a steep climb to the constant scream in the top ~45%.
  static float hunger_to_db(float h)
  {
    if (h < kHungerWakeH)
      return lerpf(kHungerMinDb, kHungerWakeDb, h / kHungerWakeH);
    if (h < kHungerBreatheH)
      return lerpf(kHungerWakeDb, kHungerBreatheDb, (h - kHungerWakeH) / (kHungerBreatheH - kHungerWakeH));
    return lerpf(kHungerBreatheDb, kHungerMaxDb, (h - kHungerBreatheH) / (1.f - kHungerBreatheH));
  }

  float hunger_to_lpf_hz(float h) const { return norm_to_exp(h, kLpfMinHz, kLpfMaxHz); }
  float release_to_seconds(float v) const { return norm_to_exp(v, kReleaseMinS, kReleaseMaxS); }

  // RATE picks the drift step from doubling divisions: 1/16, 1/8, a beat, 1/2,
  // a bar, 2 bars, 4 bars. A power curve gives the faster divisions more of the
  // control and centres 512 in the beat zone; the drift holds each target for
  // 1, 2 or 4 steps, so the slow end holds up to 16 bars.
  static uint32_t rate_to_ticks(float v)
  {
    static constexpr uint32_t kTicks[7] = {1, 2, 4, 8, 16, 32, 64};
    return kTicks[std::min(6, static_cast<int>(7.f * std::pow(v, kRateCurve)))];
  }

  // Body delay, a blend of Audrey's absolute 1-100 ms (gritty: the string fights
  // the body as pitch moves, but alone it can stick on one tone) and a multiple
  // of the string period (follows PITCH, but alone sounds like a plain oscillator).
  float body_to_delay(float v, float string_period) const
  {
    const float fixed = norm_to_exp(v, kBodyMinS, kBodyMaxS) * getSampleRate();
    const float tracked = norm_to_exp(v, kBodyMinRatio, kBodyMaxRatio) * string_period;
    const float samples = lerpf(fixed, tracked, kBodyTrack);
    return clampf(samples, 1.f, kBodyLength - 2.f);
  }

  void clear_state()
  {
    for (int i = 0; i < 2; ++i)
    {
      strings_[i].reset();
      body_[i].clear();
    }
    lpf_.reset();
    hpf_.reset();
    fwd_lpf_.reset();
    fwd_hpf_.reset();
    reverb_.reset();
    gate_ = 0.f;
  }

  Params params_;
  bool touching_ = false;

  KarplusString strings_[2];
  DelayLine<kBodyLength> body_[2];
  StereoBiquad lpf_;
  StereoBiquad hpf_;
  StereoBiquad fwd_lpf_, fwd_hpf_;
  FdnReverb reverb_;
  Overdrive drive_;
  Wander wander_[3];

  // tempo
  float tempo_bpm_ = 120.f;
  volatile uint32_t ext_ticks_ = 0;
  volatile uint32_t ext_tick_counter_ = 0;
  uint32_t ext_ticks_seen_ = 0;
  uint32_t tick_count_ = 0;
  uint32_t samples_since_ext_tick_ = 0xFFFFFF;
  float tick_phase_ = 0.f;
  WhiteNoise noise_;

  // smoothed state
  float pitch_ = 40.f;
  float string_delay_ = 1000.f;
  float lpf_hz_ = kLpfMaxHz;
  float body_pos_ = 0.5f; // smoothed BODY, 0..1
  float hunger_ = 0.f;
  float reverb_pos_ = 0.f;
  float fb_gain_ = 0.f;
  float excite_gain_ = 0.f;
  float input_env_ = 0.f;
  float sustain_env_ = 0.f;
  float fwd_gain_ = 0.f;
  float feed_age_ = 60.f; // seconds since input was last heard
  float gate_ = 0.f;
};
