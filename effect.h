/*
 *  File: effect.h
 *
 *  AUDREY: resonating feedback horror synth for NTS-3 kaoss pad kit.
 *  Based on Audrey II by Nick Donaldson / Synthux Academy (MIT).
 *
 *  input --x INPUT--+
 *  noise (-90 dB) --+
 *                   v
 *    +--> KS string -> drive -> LPF -> HPF -> reverb --+--> soft clip -> out
 *    |                                                 |
 *    +--- body delay <-- x loop gain <-----------------+
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
    LOW_CUT,
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
    float low_cut; // 0..1
    float reverb;  // 0..1
    float release; // 0..1

    void reset()
    {
      pitch = 40.f;
      hunger = 0.f;
      input = 0.f;
      body = param_10bit_to_f32(725);
      drive = 0.f;
      low_cut = param_10bit_to_f32(550);
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
    case LOW_CUT:
      params_.low_cut = param_10bit_to_f32(value);
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
    wander_[0].seed(0xA0D3E1U);
    wander_[1].seed(0x5EED2U);
    wander_[2].seed(0xB12EA7U);

    // start smoothed state at targets so loading the unit doesn't glide
    pitch_ = params_.pitch;
    string_delay_ = pitch_to_delay(pitch_);
    lpf_hz_ = hunger_to_lpf_hz(params_.hunger);
    hpf_hz_ = low_cut_to_hz(params_.low_cut);
    body_pos_ = params_.body;
    hunger_ = params_.hunger;
    reverb_pos_ = params_.reverb;
    fb_gain_ = 0.f;
    input_gain_ = 0.f;
    gate_ = 0.f;
    touching_ = false;

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

    // The gate slides linearly: a quick attack on touch, and on release it drags
    // HUNGER back down to zero over RELEASE, so the loop sinks through its
    // threshold and darkens as it goes, whatever height Y was at.
    const float gate_rate = touching_ ? 1.f / kAttackTime : -1.f / release_to_seconds(p.release);
    const float gate_end = clampf(gate_ + gate_rate * frames / sr, 0.f, 1.f);
    const float hunger_eff = hunger_ * gate_end;

    lpf_hz_ += (hunger_to_lpf_hz(hunger_eff) - lpf_hz_) * block_coef(kFilterLag, sr, frames);
    hpf_hz_ += (low_cut_to_hz(p.low_cut) - hpf_hz_) * block_coef(kFilterLag, sr, frames);
    lpf_.set_lowpass(lpf_hz_, kFilterQ, sr);
    hpf_.set_highpass(hpf_hz_, kFilterQ, sr);

    // REVERB: mix rises linearly, decay on Audrey's anti-exponential curve
    reverb_pos_ += (p.reverb - reverb_pos_) * block_coef(kFilterLag, sr, frames);
    const float reverb_mix = reverb_pos_;
    reverb_.set_feedback(lerpf(kReverbFbMin, kReverbFbMax, ftension(reverb_pos_, -3.f)));

    // string period ramps linearly across the block
    const float string_delay_target = pitch_to_delay(pitch_);
    const float string_delay_inc = (string_delay_target - string_delay_) / frames;

    // body delays drift on their own (independently per side) so resonances
    // slide in and out of alignment: the loop swells, sags and comes back
    const float wander_l = 1.f + kWanderDepth * wander_[0].process_block(frames, sr);
    const float wander_r = 1.f + kWanderDepth * wander_[1].process_block(frames, sr);
    // and the loop gain drifts a few dB, dipping the lower half of HUNGER below
    // its threshold now and then so the sound fades out and blooms back
    const float wander_gain = db_to_lin(kWanderGainDb * wander_[2].process_block(frames, sr));

    // loop gain ramps linearly across the block; the last stretch of the gate
    // fades it fully out so an untouched loop is open
    const float fb_gain_end = hunger_to_loop_gain(hunger_eff) * std::min(1.f, gate_end * kGateFadeOut) * wander_gain;
    const float fb_gain_inc = (fb_gain_end - fb_gain_) / frames;
    gate_ = gate_end;

    // ---- per-sample smoothing targets ----
    const float input_gain_target = p.input * kInputMaxGain;
    const float gain_c = onepole_coef(kGainLag, sr);
    const float body_c = onepole_coef(kBodyLag, sr);

    for (const float *out_end = out + frames * 2; out != out_end; in += 2, out += 2)
    {
      fb_gain_ += fb_gain_inc;
      input_gain_ += (input_gain_target - input_gain_) * gain_c;
      body_pos_ += (p.body - body_pos_) * body_c;
      string_delay_ += string_delay_inc;
      const float body_delay = body_to_delay(body_pos_, string_delay_);

      // ---> Feedback loop: body delay + noise + input into the string
      const float excite = noise_.process() * kNoiseAmp + (in[0] + in[1]) * 0.5f * input_gain_;
      float l = body_[0].read_linear(clampf(body_delay * wander_l, 1.f, kBodyLength - 2.f)) + excite;
      float r = body_[1].read_linear(clampf(body_delay * wander_r - 4.f, 1.f, kBodyLength - 2.f)) + excite;

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

      out[0] = soft_clip(l * kOutputGain);
      out[1] = soft_clip(r * kOutputGain);
    }

    string_delay_ = string_delay_target;
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
  static constexpr float kHpfMinHz = 10.f;
  static constexpr float kHpfMaxHz = 4000.f;
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
  static constexpr float kInputMaxGain = 2.f; // +6 dB
  static constexpr float kOutputGain = 0.5f;
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
  // regardless of DRIVE and REVERB
  float hunger_to_loop_gain(float h) const
  {
    const float db = hunger_to_db(h) + kReverbThresholdDb * reverb_pos_;
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
  float low_cut_to_hz(float v) const { return norm_to_exp(v, kHpfMinHz, kHpfMaxHz); }
  float release_to_seconds(float v) const { return norm_to_exp(v, kReleaseMinS, kReleaseMaxS); }

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
    reverb_.reset();
    gate_ = 0.f;
  }

  Params params_;
  bool touching_ = false;

  KarplusString strings_[2];
  DelayLine<kBodyLength> body_[2];
  StereoBiquad lpf_;
  StereoBiquad hpf_;
  FdnReverb reverb_;
  Overdrive drive_;
  Wander wander_[3];
  WhiteNoise noise_;

  // smoothed state
  float pitch_ = 40.f;
  float string_delay_ = 1000.f;
  float lpf_hz_ = kLpfMaxHz;
  float hpf_hz_ = 250.f;
  float body_pos_ = 0.5f; // smoothed BODY, 0..1
  float hunger_ = 0.f;
  float reverb_pos_ = 0.f;
  float fb_gain_ = 0.f;
  float input_gain_ = 0.f;
  float gate_ = 0.f;
};
