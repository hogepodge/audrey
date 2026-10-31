# AUDREY

A resonating feedback horror synth for the KORG NTS-3 kaoss pad kit, based on
[Audrey II](https://github.com/Synthux-Academy/Audrey-II) by Synthux Academy.

A Karplus-Strong string sits inside a feedback loop with drive, filters, a
reverb, and a body delay, and is fed a constant stream of inaudible noise.
Touch the pad and the loop wakes up. Lift your finger and it sinks back into
the dark.

## Playing it

| Control | Parameter | What it does |
|---|---|---|
| Pad touch | | Wakes the loop. Lifting slides HUNGER back to zero over RELEASE. |
| Pad X | PITCH | String pitch, E0 to C5, with a short glide. Same scale as the stock OSC effects (value = MIDI note x 8). |
| Pad Y | HUNGER | Loop gain and brightness. Bottom ~10% is silent. Up to about halfway it stirs and breathes, fading out and blooming back on its own. The top half is a steady scream. |
| FX DEPTH | INPUT | How much the beast yields to and feeds on your input. New sounds and attacks make it pull back so you hear your signal through its throat; held sounds let it creep back underneath. |

Editor parameters (KORG Kontrol Editor, or Effect Edit on the unit):

| Parameter | Default | What it does |
|---|---|---|
| BODY | 725 | Body delay length: which overtones ring and how they clash. |
| DRIVE | 0 | Clip shape, from a round soft clip (Audrey II's sound) to a lopsided hard clip. |
| LEVEL | 512 | Output level. 512 is -6 dB, 1023 is full. |
| REVERB | 600 | In-loop reverb mix and decay together, from dry string to cavern. |
| RELEASE | 795 | How long the loop takes to collapse after you lift, 0.1 to 20 s. |

Any parameter can be reassigned to X, Y, or DEPTH in Effect Edit.

Tips:

- Set the module's **rEL tIME** high (Module Common, EDIT + FX twice) so the
  tail isn't cut off when you lift your finger.
- **Hold** (touch + PERFORM) leaves the beast running hands-free. In the
  middle of the pad it keeps fading and returning by itself.
- Audrey II's tape echo isn't included. For that sound, put the stock Tape
  Echo after AUDREY, and add a Limiter if needed.

## Building

AUDREY builds inside the [logue SDK](https://github.com/korginc/logue-sdk).
Clone this repo into `platform/nts-3_kaoss/audrey`, initialize the CMSIS
submodule (`git submodule update --init platform/ext/CMSIS`), then from the
logue-sdk root:

```
docker/build_image.sh                                  # once
docker/run_cmd.sh build --clean nts-3_kaoss/audrey     # header-only changes need --clean
docker/run_cmd.sh build nts-3_kaoss/audrey
```

This produces `audrey.nts3unit`. Load it into a user effect slot with KORG
Kontrol Editor.

To try it in a browser, run `make wasm` in this directory. This needs the
logue SDK's emsdk submodule installed and activated. The sim works in Safari;
Brave stays silent.

## Credits

- **Audrey II**: concept and firmware by Nick Donaldson, visual and hardware
  design by Roey Tsemah, published by Synthux Academy (MIT). AUDREY's
  architecture, string model, filters, and many default values come from it.
- **DaisySP** by Electrosmith (MIT): the string, filter, and overdrive modules
  Audrey II uses are reimplemented here.
- **reverbsc** by Sean Costello: the in-loop reverb follows its design and
  parameters.
- **KORG logue SDK** (BSD 3-Clause): the build system and unit template, plus
  helpers by Shijie Xia from the `pluck` example.

See [NOTICE.md](NOTICE.md) for details and full license texts.

## License

MIT, copyright (c) 2026 Chris Hoge. See [LICENSE](LICENSE). Files from the
KORG logue SDK stay under the BSD 3-Clause License, as marked in each file.
