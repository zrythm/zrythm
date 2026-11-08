// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <string_view>

#include "dsp/processor_base.h"
#include "utils/units.h"
#include "utils/utf8_string.h"

namespace zrythm::plugins::modulators
{

// Helpers shared by the modulator test sources. The definitions are
// inline and at namespace scope: unity builds concatenate the test
// sources into one translation unit, where per-file anonymous
// namespaces merge and duplicate definitions collide.
inline constexpr auto kSampleRate = units::sample_rate (44100);
inline constexpr auto kBlockLength = units::samples (256);

inline dsp::ProcessorParameter *
find_parameter (dsp::ProcessorBase &processor, std::string_view unique_id)
{
  const auto found = std::ranges::find_if (
    processor.get_parameters (), [unique_id] (const auto &param_ref) {
      auto * param = param_ref.get ();
      return param != nullptr
             && param->get_unique_id ()
                  == dsp::ProcessorParameter::UniqueId (
                    utils::Utf8String::from_utf8_encoded_string (unique_id));
    });
  return found != processor.get_parameters ().end () ? found->get () : nullptr;
}

} // namespace zrythm::plugins::modulators
