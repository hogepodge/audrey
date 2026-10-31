/*
    BSD 3-Clause License    Copyright (c) 2026, KORG INC.
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

//*/

/*
 *  File: header.c
 *
 *  NTS-3 kaoss pad kit generic effect unit header definition
 *
 */

#include "unit_genericfx.h"   // Note: Include base definitions for genericfx units
// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),           // Size of this header. Leave as is.
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx, // Target platform and module pair for this unit
    .api = UNIT_API_VERSION,                                  // API version for which unit was built. See runtime.h
    .dev_id = 0x686F6765U,                                    // Developer ID ('hoge')
    .unit_id = 0x1U,                                          // ID for this unit. Scoped within the context of a given dev_id.
    .version = 0x00000200U,                                   // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "AUDREY",                                         // Name for this unit, will be displayed on device
    .num_params = 8,                                          // Number of valid parameter descriptors. (max. 8)

    .params = {
      // Format: min, max, center (unused), default, type, frac. bits, frac. mode, <reserved>, name
      // See common/runtime.h for type enum and unit_param_t structure

      {0, 1023, 0, 320, k_unit_param_type_none, 0, 0, 0, {"PITCH"}},   // MIDI note x 8 (192 = C1)
      {0, 1023, 0, 0,   k_unit_param_type_none, 0, 0, 0, {"HUNGER"}},  // loop gain + LPF macro
      {0, 1023, 0, 0,   k_unit_param_type_none, 0, 0, 0, {"INPUT"}},   // input into the loop
      {0, 1023, 0, 725, k_unit_param_type_none, 0, 0, 0, {"BODY"}},    // body delay: 1-100 ms blended with 0.5-8 string periods
      {0, 1023, 0, 0,   k_unit_param_type_none, 0, 0, 0, {"DRIVE"}},   // clip shape: round -> lopsided hard
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"LEVEL"}},   // output amplitude (512 = -6 dB)
      {0, 1023, 0, 600, k_unit_param_type_none, 0, 0, 0, {"REVERB"}},  // in-loop reverb macro
      {0, 1023, 0, 795, k_unit_param_type_none, 0, 0, 0, {"RELEASE"}}, // loop collapse 0.1-20 s
    },
  },
  .default_mappings = {
    // Format: assign, curve, curve polarity, min, max, default value
    {k_genericfx_param_assign_x,     k_genericfx_curve_linear, k_genericfx_curve_unipolar, 128, 576, 320},  // PITCH E0-C5
    {k_genericfx_param_assign_y,     k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 0},     // HUNGER
    {k_genericfx_param_assign_none,  k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 0},     // INPUT
    {k_genericfx_param_assign_none,  k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 725},   // BODY
    {k_genericfx_param_assign_none,  k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 0},     // DRIVE
    {k_genericfx_param_assign_depth, k_genericfx_curve_exp,    k_genericfx_curve_unipolar, 0, 1023, 512},   // LEVEL, like Korg's DRY/WET at 512
    {k_genericfx_param_assign_none,  k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 600},   // REVERB
    {k_genericfx_param_assign_none,  k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 795},   // RELEASE
  }
};
