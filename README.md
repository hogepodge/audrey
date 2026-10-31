# AUDREY

A resonating feedback horror synth for the KORG NTS-3 kaoss pad kit, based on
[Audrey II](https://github.com/Synthux-Academy/Audrey-II) by Synthux Academy.

AUDREY makes no sound on its own until you touch the pad. A Karplus-Strong
string sits inside a feedback loop with drive, filters, a reverb, and a body
delay, and is fed a constant stream of inaudible noise. Touch the pad and the
loop wakes up and grows. Move your finger and it howls, groans, and drifts.
Lift it and the beast sinks back into the dark.

```
 noise --+     your input --x INPUT (also makes the beast yield)
         v                 v
   +--> string --------> + --> drive -> filters -> reverb --+--> LEVEL -> out
   |                                                        |
   +---- body delay <---- loop gain (HUNGER, touch) <-------+
```

## Contents

- [Download](#download)
- [Building](#building)
- [Installing on the NTS-3](#installing-on-the-nts-3)
- [Setting up a program](#setting-up-a-program)
- [Playing](#playing)
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
| FX DEPTH | INPUT | How much the beast feeds on and yields to your input. |

**HUNGER, bottom to top:**

- **Bottom ~10%:** asleep. Silent, with an occasional faint stir.
- **Up to about halfway:** restless. The beast fades out and blooms back on
  its own, even with your finger still. Low notes on the left take their
  time to build.

The drift behind this motion follows the NTS-3's tempo (tap tempo, MIDI clock
or sync). Its changes land on the beat, every 1, 2 or 4 beats at random, so
the swells and fades fall in time with the rest of your set.
- **Top half:** a steady scream that gets brighter and wilder as you climb.
  The timbre still drifts underneath.

**INPUT (FX DEPTH):**

- **Bottom of the strip:** the beast alone.
- **Higher on the strip:** whatever you send into the NTS-3 is fed through
  the beast's throat (drive, filters, reverb) and seeds the loop.
- **New notes and attacks:** the beast pulls back so you hear yourself, then
  grows back out of what you fed it.
- **Held notes and drones:** the beast creeps back underneath.
- **If your input keeps drowning the beast,** slide the strip down.

**Tips:**

- **Hold** (touch the pad, then press **PERFORM**) leaves the beast running
  hands-free. Parked in the restless zone, it keeps fading and returning by
  itself. Long-press PERFORM to release.
- Sweep slowly. The loop takes a moment to re-lock after big jumps, and the
  transitions are part of the sound.
- Raise **REVERB** to make the restless zone fuller and the release longer.

## Parameters

Every parameter can be reassigned to X, Y, or FX DEPTH, with its own range
and curve, in Effect Edit (hold **EDIT**, tap **FX1**) or in KORG KONTROL
EDITOR.

| Parameter | Default | Assigned to | What it does |
|---|---|---|---|
| PITCH | 320 | X (128-576) | String pitch. Same scale as the stock OSC effects: value = MIDI note x 8 (192 = C1, 768 = C7). |
| HUNGER | 0 | Y | Loop gain and brightness. Wake-up point stays put regardless of DRIVE and REVERB. |
| INPUT | 512 | FX DEPTH | How much of your input feeds the loop, and how strongly the beast yields to it. |
| BODY | 725 | - | Body delay length: which overtones ring and how they clash. |
| DRIVE | 0 | - | Clip shape, from round and symmetric (Audrey II's sound) to hard and lopsided, with more even harmonics. |
| LEVEL | 512 | - | Output level. 512 is -6 dB, 1023 is full. |
| REVERB | 600 | - | In-loop reverb, mix and decay together, from a dry string to a cavern. |
| RELEASE | 795 | - | How long the loop takes to collapse after you lift, 0.1 to 20 s (795 is about 6 s). |

## Troubleshooting

- **No sound when touching the pad.** Check that you're above the bottom ~10%
  of Y, that the module is on, and that LEVEL isn't at 0.
- **The sound cuts off when I lift my finger.** Raise **rEL tIME** in Module
  Common.
- **Too loud or too quiet.** Lower or raise LEVEL, or use the module's
  **Out Gain** (Module Common, -12 to +12 dB).
- **My input does nothing.** INPUT (FX DEPTH) may be at the bottom, or the
  NTS-3's input gain may be low (Global settings: hold EDIT at power-on).
- **The beast won't come back while I play.** Slide FX DEPTH down. Constant
  new attacks keep it yielding.

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
