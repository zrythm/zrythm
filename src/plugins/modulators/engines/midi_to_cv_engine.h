// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/modulators/engines/midi_note_tracker.h"
#include "plugins/modulators/modulator_engine.h"
#include "plugins/plugin_descriptor.h"

namespace zrythm::plugins::modulators
{

/**
 * @brief MIDI to CV modulator engine.
 *
 * Converts performed notes to CV: the pitch output carries the held note
 * (0-1 across the MIDI note range), the gate output is 1 while a note is
 * held and 0 otherwise, and the velocity output carries the last note-on
 * velocity. Note handling is monophonic; the most recent note-on wins.
 */
class MidiToCvEngine final : public ModulatorEngine
{
public:
  static constexpr std::string_view kDescriptorId{
    "zrythm.modulators.midi_to_cv"
  };
  static constexpr std::string_view kInputSymbol{ "midi_in" };
  static constexpr std::string_view kPitchSymbol{ "pitch" };
  static constexpr std::string_view kGateSymbol{ "gate" };
  static constexpr std::string_view kVelocitySymbol{ "velocity" };

  static std::unique_ptr<PluginDescriptor> make_descriptor ();

  [[nodiscard]] ModulatorLayout layout () const override;
  void prepare (const ModulatorRuntimeContext &context) noexcept override;
  void release_resources () noexcept override;
  void process (
    dsp::graph::ProcessBlockInfo time_info,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept [[clang::nonblocking]] override;

private:
  dsp::MidiPort * midi_in_{};
  dsp::CVPort *   pitch_out_{};
  dsp::CVPort *   gate_out_{};
  dsp::CVPort *   velocity_out_{};
  MidiNoteTracker tracker_;
  int             held_note_{ -1 };
  float           held_velocity_{};
};

} // namespace zrythm::plugins::modulators
