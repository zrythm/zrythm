// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "dsp/audio_port.h"
#include "dsp/cv_port.h"
#include "dsp/midi_port.h"
#include "dsp/parameter.h"

namespace zrythm::plugins::modulators
{

/**
 * @brief Prepared ports and parameters of a native modulator, in layout order.
 *
 * Built by the Modulator host during preparation; engines cache the pointers
 * they need in prepare() and must not retain the context itself.
 */
class ModulatorRuntimeContext final
{
public:
  [[nodiscard]] dsp::ProcessorParameter *
  parameter (size_t index) const noexcept
  {
    return index < parameters_.size () ? parameters_[index] : nullptr;
  }

  [[nodiscard]] dsp::CVPort * cv_input (size_t index) const noexcept
  {
    return index < cv_inputs_.size () ? cv_inputs_[index] : nullptr;
  }

  [[nodiscard]] dsp::CVPort * cv_output (size_t index) const noexcept
  {
    return index < cv_outputs_.size () ? cv_outputs_[index] : nullptr;
  }

  [[nodiscard]] dsp::AudioPort * audio_input (size_t index) const noexcept
  {
    return index < audio_inputs_.size () ? audio_inputs_[index] : nullptr;
  }

  [[nodiscard]] dsp::MidiPort * midi_input (size_t index) const noexcept
  {
    return index < midi_inputs_.size () ? midi_inputs_[index] : nullptr;
  }

private:
  friend class Modulator;

  std::vector<dsp::ProcessorParameter *> parameters_;
  std::vector<dsp::CVPort *>             cv_inputs_;
  std::vector<dsp::CVPort *>             cv_outputs_;
  std::vector<dsp::AudioPort *>          audio_inputs_;
  std::vector<dsp::MidiPort *>           midi_inputs_;
};

} // namespace zrythm::plugins::modulators
