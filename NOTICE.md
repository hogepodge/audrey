# Notices and credits

AUDREY is MIT-licensed (see [LICENSE](LICENSE)), copyright (c) 2026 Chris Hoge.
It is built on the work of the projects below. Their copyright notices and
license texts are reproduced here as their licenses require.

## Audrey II, by Synthux Academy

AUDREY is a port of Audrey II, a droning horror synth for the Daisy Seed and
the Synthux Simple platform.

- Concept and firmware: [Nick Donaldson](https://github.com/ndonald2)
- Visual and hardware design: [Roey Tsemah](https://github.com/roeytsemah)
- Source: https://github.com/Synthux-Academy/Audrey-II

Derived from Audrey II:

- The architecture: a Karplus-Strong string excited by constant inaudible
  noise, inside an outer feedback loop with drive, lowpass/highpass filters,
  reverb, and a body delay.
- `dsp/ks_string.hpp`: a port of Audrey II's `KarplusString`.
- `dsp/filters.hpp`: the biquad coefficient formulas from Audrey II's
  `BiquadSection`.
- `dsp/utility.hpp`: `ftension`, the curve Audrey II uses for its reverb
  knob.
- Default values and ranges: noise level, filter Q and cutoffs, overdrive
  amount, body delay range, reverb feedback range and damping, and pitch glide.

```
MIT License

Copyright (c) 2026 Synthux Academy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## DaisySP, by Electrosmith

Audrey II is built on DaisySP. AUDREY does not link against DaisySP, but
reimplements the DaisySP modules Audrey II uses:

- `KarplusString` (via Audrey II's modified version), in `dsp/ks_string.hpp`
- `Tone` and `DcBlock`, in `dsp/filters.hpp`
- `Overdrive` and `SoftClip`, in `dsp/utility.hpp`
- `DelayLine::ReadHermite` interpolation, in `dsp/delayline.hpp`

Source: https://github.com/electro-smith/DaisySP

```
MIT License

Copyright (c) 2020 Electrosmith, Corp

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## KORG logue SDK

The unit is built with the KORG logue SDK for the NTS-3 kaoss pad kit.

These files come from the SDK's `dummy-genericfx` and `pluck` templates and
remain under the BSD 3-Clause License:

- `Makefile`, `config.mk`, `unit.cc`, `wasm.cc`
- `header.c`: Korg's template structure, filled in with AUDREY's parameters

`dsp/utility.hpp` begins with helpers from the `pluck` example (`note_to_hz`,
`lerpf`, `clampf`, `fast_tanh`, by Shijie Xia, KORG INC.). Its other contents
are AUDREY's own work.

Source: https://github.com/korginc/logue-sdk

```
BSD 3-Clause License

Copyright (c) 2018, KORG INC.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## reverbsc, by Sean Costello

The in-loop reverb (`dsp/reverb.hpp`) takes its design from Sean Costello's
reverbsc, which Audrey II uses through DaisySP's `ReverbSc`. AUDREY uses the
same number of delay lines, reverbsc's base delay times and modulation rates,
and one-pole damping in each line. No reverbsc code is used: the feedback
matrix, modulation, and level compensation are AUDREY's own implementation.

## Tools

Developed with help from Claude (Anthropic), which co-authored the commits.
