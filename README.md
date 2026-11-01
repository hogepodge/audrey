# AUDREY

A resonating feedback horror synth for the KORG NTS-3 kaoss pad kit, based on
[Audrey II](https://github.com/Synthux-Academy/Audrey-II) by Synthux Academy.

AUDREY makes no sound on its own until you touch the pad. A Karplus-Strong
string sits inside a feedback loop with drive, filters, a reverb, and a body
delay, and is fed a constant stream of inaudible noise. Touch the pad and the
loop wakes up and grows. Move your finger and it howls, groans, and drifts.
Lift it and the beast sinks back into the dark.

```
      your input (INPUT: level-capped, and makes the beast hungrier)
 noise --+  |                                 your input, direct (rises with HUNGER)
         v  v                                         v
   +---> + --> string --> drive -> filters -> reverb --+--> + --> out
   |                                                   |
   +---- body delay <---- loop gain (HUNGER, touch) <--+
```

## Contents

- [Download](#download)
- [Building](#building)
- [Installing on the NTS-3](#installing-on-the-nts-3)
- [Setting up a program](#setting-up-a-program)
- [Playing](#playing)
- [Patches](#patches)
- [Parameters](#parameters)
- [Troubleshooting](#troubleshooting)
- [Trying it in a browser](#trying-it-in-a-browser)
- [Credits](#credits)
- [License](#license)

## Download

A ready-to-install `audrey.nts3unit` is attached to each release on the
[Releases page](https://github.com/hogepodge/audrey/releases). Download it and
skip ahead to [Installing on the NTS-3](#installing-on-the-nts-3), or build it
yourself as below.

## Building

AUDREY is built inside KORG's [logue SDK](https://github.com/korginc/logue-sdk).
The result is a single file, `audrey.nts3unit`.

### 1. Get the logue SDK and AUDREY

```sh
git clone https://github.com/korginc/logue-sdk.git
cd logue-sdk
git submodule update --init platform/ext/CMSIS
git clone https://github.com/hogepodge/audrey.git platform/nts-3_kaoss/audrey
```

AUDREY has to live at `platform/nts-3_kaoss/audrey`, because its Makefile
finds the SDK by relative path.

### 2a. Build with Docker (recommended)

Requires [Docker](https://www.docker.com/). From the `logue-sdk` directory:

```sh
docker/build_image.sh                          # first time only, takes a few minutes
docker/run_cmd.sh build nts-3_kaoss/audrey
```

The unit is written to `platform/nts-3_kaoss/audrey/audrey.nts3unit`.

If you change a header file (`effect.h` or anything in `dsp/`), clean first.
Otherwise the build may silently reuse old object files:

```sh
docker/run_cmd.sh build --clean nts-3_kaoss/audrey
docker/run_cmd.sh build nts-3_kaoss/audrey
```

### 2b. Build without Docker

Install the ARM toolchain the NTS-3 needs (gcc-arm-none-eabi 10.3-2021.10)
into the SDK. On macOS:

```sh
cd tools/gcc && ./get_gcc_macos.sh && cd ../..
```

Linux and Windows scripts are in `tools/gcc`. Then build and package:

```sh
cd platform/nts-3_kaoss/audrey
make
make install
```

## Installing on the NTS-3

1. Connect the NTS-3 to your computer over USB.
2. Open **KORG KONTROL EDITOR** and let it find the NTS-3.
3. Back up your library file first (in Finder: File, Duplicate). Avoid
   receiving from the unit into a library you haven't saved.
4. In the editor's user effects, add `audrey.nts3unit` to a free slot and
   send it to the NTS-3.

Loaded units appear at the end of the effect list. The NTS-3 remembers
AUDREY in your programs by its developer and unit ID, so moving it to
another slot later is fine.

## Setting up a program

1. In Perform mode, press **FX1** and turn the knob to **AUDREY**. Press the
   knob to turn the module on.
2. Hold **EDIT** and tap **FX1** twice to open Module Common. Set
   **rEL tIME** high (1023), or the NTS-3 will cut the beast off when you
   lift your finger instead of letting it die out.
3. Choose a **Release Mode**:
   - **THRU**: your dry input comes back when you lift.
   - **SILENT**: only the beast is heard, for using AUDREY as a pure sound
     source.
4. Optional extras in the other modules:
   - **Tape Echo** after AUDREY, for Audrey II's warped echoes.
   - A **Limiter** at the end of the chain.
5. Save with **EDIT**, then **SAVE ProG**.

## Playing

| Control | Parameter | What it does |
|---|---|---|
| Touch the pad | | Wakes the loop. Lifting lets it sink away over RELEASE. |
| Pad X | PITCH | The string's pitch, E0 to C5, with a short glide. |
| Pad Y | HUNGER | How hungry the loop is. See below. |
| FX DEPTH | INPUT | How much the beast feeds on your input. |

**HUNGER, bottom to top:**

- **Bottom ~10%:** asleep. Silent, with an occasional faint stir.
- **Up to about halfway:** restless. The beast fades out and blooms back on
  its own, even with your finger still. Low notes on the left take their
  time to build.
- **Top half:** a steady scream that gets brighter and wilder as you climb.
  The timbre still drifts underneath.

The drift behind this motion follows the NTS-3's tempo (tap tempo, MIDI clock
or sync). Its changes land on the grid, every 1, 2 or 4 steps at random, so
the swells and fades fall in time with the rest of your set. RATE sets the
step, from a 16th note (a nervous warble) to 4 bars (glacial tides).

**INPUT (FX DEPTH):**

- **Bottom of the strip:** the beast alone.
- **Higher on the strip:** the beast feeds on whatever you send into the
  NTS-3. Your input goes into its string, as in AudreyTouch, and circulates
  through the string and reverb, so hits, notes and phrases grow into tones
  at the pitch under your finger.
- **Feeding makes it hungrier:** while input arrives, the loop is lifted to
  just under the point where it sings by itself, so what you feed it keeps
  circulating. The lift holds for 2 s after the input stops, then lets go
  over 2 s.
- **Balance:** your input's level inside the loop is capped, the same however
  loud you play, so FX DEPTH sets the balance:
  - **Up to halfway:** the beast stays on top and your input only feeds it.
  - **Around three quarters:** your input and the beast are about even.
  - **Top:** your input takes over the loop.

  In the scream zone (top of Y) the beast wins anywhere on the strip.
- **Your input is also heard directly,** through the beast's filters but
  outside its loop, so it can't take the beast over. It is faint at the bottom
  of Y and rises with HUNGER to sit just above the beast's chaos at the top.
  For your fully dry input, run a parallel chain on the NTS-3.

**Tips:**

- **Hold** (touch the pad, then press **PERFORM**) leaves the beast running
  hands-free. Parked in the restless zone, it keeps fading and returning by
  itself. Long-press PERFORM to release.
- Sweep slowly. The loop takes a moment to re-lock after big jumps, and the
  transitions are part of the sound.
- Raise **REVERB** to make the restless zone fuller and the release longer.
- Assign **RATE** to FX DEPTH (Effect Edit) to throw the drift between slow
  tides and a fast warble while you play.

## Patches

Starting points found by playing AUDREY on the NTS-3. Each uses two AUDREY
modules routed in parallel, each with its own parameters. Three or more
AUDREYs at once currently overload the NTS-3 and glitch, so stick to two.

### Menacing: two beasts, two echoes

- **Chain:** two AUDREYs in parallel, each followed by its own **Tape Echo**.
- **Setup:** give the two AUDREYs different parameters, so they grind against
  each other rather than doubling up.
- This is Audrey II's full signal flow, tape echo included, times two.

### Mysterious: low and high

- **Chain:** two AUDREYs in parallel, then a **Flanger** and a **Tape Echo**.
- **Setup:** in Effect Edit, give each AUDREY a different PITCH range on X:
  one tuned down, one tuned up. Every finger movement then plays a low drone
  and a high cry together.
- **Freeze** the flanger and the echo to hold a moment in place while both
  beasts keep breathing and drifting underneath.

### Ways to vary them

- **Different RATE on each AUDREY** (for example a beat and 2 bars), so
  their swells drift in and out of phase instead of breathing together.
- **Different BODY settings,** so each reinforces different overtones.
- **DRIVE up on one,** for a hard, lopsided voice against a round one.
- **An interval between the PITCH ranges,** such as a tritone or a minor
  second, for something nastier.

## Parameters

AUDREY has eight parameters. Each runs from 0 to 1023. Any of them can be
assigned to pad X, pad Y or FX DEPTH, with its own range and curve, in
Effect Edit (hold **EDIT**, tap **FX1**) or in KORG KONTROL EDITOR.
Unassigned parameters keep the value set there.

| Parameter | Default | Assigned to | In short |
|---|---|---|---|
| [PITCH](#pitch) | 320 (E2) | X, 128-576 | The string's pitch |
| [HUNGER](#hunger) | 0 | Y, 0-1023 | Loop gain and brightness |
| [INPUT](#input) | 512 | FX DEPTH, 0-1023 | How much the beast feeds on your input |
| [BODY](#body) | 725 | - | Body delay length: which overtones ring |
| [DRIVE](#drive) | 0 | - | Clip shape, round to hard and lopsided |
| [RATE](#rate) | 512 (a beat) | - | Speed of the drift, locked to tempo |
| [REVERB](#reverb) | 600 | - | In-loop reverb, mix and decay together |
| [RELEASE](#release) | 795 (about 6 s) | - | How long the loop takes to collapse after you lift |

### PITCH

The pitch of the Karplus-Strong string at the heart of the loop.

- **Scale:** value = MIDI note x 8, the same as the stock OSC effects. Every
  8 steps is a semitone.

  | Value | 128 | 192 | 320 | 384 | 480 | 576 |
  |---|---|---|---|---|---|---|
  | Note | E0 (21 Hz) | C1 | E2 (82 Hz) | C3 | C4 | C5 (523 Hz) |

- **Default mapping:** X, from 128 (E0) at the left to 576 (C5) at the right.
  Narrow or shift the range in Effect Edit to play a smaller span, or to
  tune two AUDREYs apart.
- **Glide:** pitch changes slide over about 40 ms.
- The beast doesn't only sing at this pitch. The body delay and the filters
  pick out overtones and clashing tones around it, so treat PITCH as where
  the beast lives rather than the note it plays.

### HUNGER

How hungry the loop is: its feedback gain and brightness together.

- **Default mapping:** Y, 0 at the bottom to 1023 at the top.
- **Gain:** the loop is silent at the bottom and climbs through three zones:

  | Range | Zone | What you hear |
  |---|---|---|
  | 0 to about 100 | Asleep | Silent, with an occasional faint stir |
  | about 100 to 560 | Restless | Fades out and blooms back on its own |
  | about 560 to 1023 | Scream | A steady, saturated scream, wilder as you climb |

  The point where the beast wakes stays at the same height whatever DRIVE
  and REVERB are set to: AUDREY compensates the loop gain for both.
- **Brightness:** a lowpass filter in the loop opens with HUNGER:

  | Value | 0 | 256 | 512 | 768 | 1023 |
  |---|---|---|---|---|---|
  | Lowpass | 800 Hz | 1.7 kHz | 3.8 kHz | 8.3 kHz | 18 kHz |

- **Touch:** HUNGER only acts while you touch the pad. A touch brings it up
  in 30 ms; lifting slides it back to zero over RELEASE, so the beast sinks
  through its zones and darkens as it goes.
- The direct path of your input (see [INPUT](#input)) also rises with
  HUNGER.

### INPUT

How much the beast feeds on the audio you send into the NTS-3.

- **Default mapping:** FX DEPTH, 0 to 1023, default 512.
- **Into the loop:** your input goes into the beast's string, as in
  AudreyTouch, and circulates through the string and reverb, so hits, notes
  and phrases grow into tones at the pitch under your finger.
- **Level cap:** the input's sustained level in the loop is capped, the same
  however loud you play, so INPUT sets the balance rather than your input
  gain. Quiet input is boosted by up to 12 dB; hits may peak well over the
  cap, so they still get through, while drones and held notes are held
  under it.

  | Value | Balance (restless zone) |
  |---|---|
  | 0 | The beast alone |
  | up to 512 | The beast stays on top; your input only feeds it |
  | about 830 | Your input and the beast are about even |
  | 1023 | Your input takes over the loop |

  In the scream zone (top of Y) the beast wins anywhere on the strip.
- **Lift:** while input arrives (above about -40 dBFS), the loop gain is
  held just under the point where the beast sings by itself, so what you
  feed in keeps circulating. The lift holds for 2 s after the input stops,
  then fades over 2 s. It is full from 512 up and fades out below.
- **Direct path:** your input is also heard directly, through copies of the
  loop's filters but outside the loop, so it can't take the beast over. It
  rises with HUNGER, from -28 dB at the bottom of Y to -6.6 dB at the top,
  and is full from 512 up. For your fully dry input, run a parallel chain
  on the NTS-3.

### BODY

The length of the body delay that carries the loop back to the string. It
decides which overtones ring and how they clash with the string's pitch.

- **Range:** a blend of 79% absolute time and 21% string periods, so it both
  fights the string as PITCH moves (grit) and follows it (so it doesn't stick
  on one tone):

  | Value | 0 | 512 | 725 (default) | 1023 |
  |---|---|---|---|---|
  | Absolute part | 1 ms | 10 ms | 26 ms | 100 ms |
  | String-period part | 0.5 periods | 2 periods | 3.6 periods | 8 periods |

- **Smoothing:** changes glide over about 0.2 s, so sweeping BODY bends the
  sound rather than clicking.
- **Drift:** the left and right body delays wander independently by up to
  12% on the tempo grid (see [RATE](#rate)), and the right side is a few
  samples shorter, for width.

### DRIVE

The shape of the loop's clipper. The amount of drive is fixed, as in
Audrey II; this changes its character.

- **0:** round, symmetric soft clipping, Audrey II's sound.
- **Higher:** blends toward hard clipping with a growing offset, so the
  clipping becomes lopsided and adds even harmonics: harsher and more
  buzzing.
- The point where the beast wakes stays put as DRIVE changes.

### RATE

The speed of the drift that makes the beast breathe: the body delays wander
by up to 12% and the loop gain by up to 6 dB, which is what makes the
restless zone fade and bloom.

- **Locked to tempo:** every change lands on the NTS-3's tempo grid (tap
  tempo, MIDI clock or sync, or the internal tempo). RATE sets the grid step,
  and each change holds for 1, 2 or 4 steps at random, gliding over half of
  that.

  | Value | 0-275 | 276-439 | 440-577 | 578-701 | 702-815 | 816-921 | 922-1023 |
  |---|---|---|---|---|---|---|---|
  | Step | 1/16 | 1/8 | beat (default) | 1/2 | 1 bar | 2 bars | 4 bars |
  | Holds at 120 BPM | 0.13-0.5 s | 0.25-1 s | 0.5-2 s | 1-4 s | 2-8 s | 4-16 s | 8-32 s |

- The fast end is a nervous warble; the slow end is glacial tides. At 2 and
  4 bars, a fade in the restless zone can leave the beast quiet for 10-30 s.
- Assign RATE to FX DEPTH to throw the drift between speeds while you play.

### REVERB

The reverb inside the loop, mix and decay on one control.

- **0:** a dry string.
- **Higher:** more of the loop passes through the reverb (the mix rises in
  step with the value) and its tail grows, from short to about 0.97 feedback
  at the top: a cavern.
- **600 (default):** about 60% wet, with a long, haunting tail.
- Because the reverb is in the loop, it changes the beast itself: it makes
  the restless zone fuller and the release longer. The point where the beast
  wakes stays put as REVERB changes.

### RELEASE

How long the loop takes to collapse after you lift your finger.

| Value | 0 | 256 | 512 | 795 (default) | 1023 |
|---|---|---|---|---|---|
| Release | 0.1 s | 0.4 s | 1.4 s | about 6 s | 20 s |

- Over this time HUNGER slides linearly from where it was down to zero, so
  the beast sinks through its zones, darkening as it goes, and the loop
  opens completely in the last tenth.
- Set the module's **rEL tIME** (Module Common) high too, or the NTS-3 cuts
  AUDREY off before the release can play out.

### Fixed settings

These are built in, not adjustable:

| Setting | Value | Notes |
|---|---|---|
| Noise into the string | -90 dBFS | Inaudible; what the beast grows from, as in Audrey II |
| Highpass in the loop | 250 Hz | Audrey II's default; add a stock filter after AUDREY for more |
| Drive amount | 0.4 | As in Audrey II; DRIVE changes its shape |
| Output level | -6 dB | With 6 dB of headroom before the output soft clip |

### Module settings that matter

These belong to the NTS-3 module AUDREY runs in (hold **EDIT**, tap
**FX1** twice):

- **rEL tIME:** set high (1023) so RELEASE can play out.
- **Release Mode:** THRU brings your dry input back when you lift; SILENT
  leaves only the beast.
- **Out Gain:** -12 to +12 dB, to trim AUDREY's level.

## Troubleshooting

- **No sound when touching the pad.** Check that you're above the bottom ~10%
  of Y and that the module is on.
- **The sound cuts off when I lift my finger.** Raise **rEL tIME** in Module
  Common.
- **Too loud or too quiet.** Use the module's **Out Gain** (Module Common,
  -12 to +12 dB). AUDREY's own output sits at -6 dB.
- **My input does nothing.** INPUT (FX DEPTH) may be at the bottom, or the
  NTS-3's input gain may be low (Global settings: hold EDIT at power-on).

## Trying it in a browser

The logue SDK includes a browser simulator for testing without the hardware.
It needs the SDK's emsdk submodule:

```sh
git submodule update --init tools/emsdk
cd tools/emsdk && ./emsdk install latest && ./emsdk activate latest && cd ../..
cd platform/nts-3_kaoss/audrey
make wasm
```

`make wasm` tries to open Chrome. If you don't have Chrome, the build still
completes; serve the `sim` directory yourself:

```sh
cd sim
../../../../tools/emsdk/upstream/emscripten/emrun --no_browser --port 6931 audrey.html
```

Then open http://localhost:6931/audrey.html. Click **Toggle playback** first,
and hold the mouse button down on the pad to play. Tested in Safari; KORG
targets Chrome. Brave stays silent, probably because of its fingerprinting
protection.

## Credits

AUDREY stands on the work of others:

- **[Audrey II](https://github.com/Synthux-Academy/Audrey-II)**, published by
  Synthux Academy: concept and firmware by
  [Nick Donaldson](https://github.com/ndonald2), visual and hardware design
  by [Roey Tsemah](https://github.com/roeytsemah). AUDREY is a port of its
  architecture, and its string model, filters, and many default values come
  from it. (MIT)
- **[AudreyTouch](https://github.com/Synthux-Academy/AudreyTouch)**, Audrey
  for the Synthux Simple Touch, by Alberto Berera and Rosa Schuurmans, with
  USB MIDI by Vlad Litvinenko: the idea of feeding the input into the string, and the fast random body modulation that RATE's short
  steps echo. No AudreyTouch code is used.
- **[DaisySP](https://github.com/electro-smith/DaisySP)** by Electrosmith:
  the string, tone, DC blocker, overdrive, soft clip, and delay-interpolation
  modules Audrey II uses are reimplemented here. (MIT)
- **reverbsc** by Sean Costello: the in-loop reverb follows its design,
  delay times, and modulation rates. No reverbsc code is used.
- **[KORG logue SDK](https://github.com/korginc/logue-sdk)**: the build
  system, unit template, browser simulator, and helpers by Shijie Xia from
  the `pluck` example. (BSD 3-Clause)

Developed by Chris Hoge with Claude (Anthropic).

[NOTICE.md](NOTICE.md) lists exactly what came from where, with each
project's full license text.

## License

MIT, copyright (c) 2026 Chris Hoge. See [LICENSE](LICENSE).

Files from the KORG logue SDK (`Makefile`, `config.mk`, `unit.cc`, `wasm.cc`,
and the `header.c` template) remain under the BSD 3-Clause License, as marked
in each file.
