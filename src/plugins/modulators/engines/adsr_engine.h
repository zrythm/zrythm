// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/modulators/engines/midi_note_tracker.h"
#include "plugins/modulators/modulator_engine.h"
#include "plugins/plugin_descriptor.h"

namespace zrythm::plugins::modulators
{

/**
 * @brief ADSR modulator engine.
 *
 * Emits a 0-1 envelope gated by performed notes: a note-on starts the
 * attack/decay/sustain ramp from the current level, and the note's end starts
 * the release ramp towards zero. Ramp segments are linear with per-segment
 * time parameters; retriggering a note while one is held restarts the attack
 * from the current level. Note handling is monophonic: the most recent
 * note-on wins, and its note-off starts the release even while an earlier
 * note is still held.
 */
class AdsrEngine final : public ModulatorEngine
{
public:
  static constexpr std::string_view kDescriptorId{ "zrythm.modulators.adsr" };
  static constexpr std::string_view kAttackParameterId{
    "/zrythm/modulators/adsr/attack"
  };
  static constexpr std::string_view kDecayParameterId{
    "/zrythm/modulators/adsr/decay"
  };
  static constexpr std::string_view kSustainParameterId{
    "/zrythm/modulators/adsr/sustain"
  };
  static constexpr std::string_view kReleaseParameterId{
    "/zrythm/modulators/adsr/release"
  };
  static constexpr std::string_view kInputSymbol{ "midi_in" };
  static constexpr std::string_view kOutputSymbol{ "cv_out" };

  static std::unique_ptr<PluginDescriptor> make_descriptor ();

  [[nodiscard]] ModulatorLayout layout () const override;
  void prepare (const ModulatorRuntimeContext &context) noexcept override;
  void release_resources () noexcept override;
  void process (
    dsp::graph::ProcessBlockInfo time_info,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept [[clang::nonblocking]] override;

private:
  enum class Stage : std::uint8_t
  {
    Off,
    Attack,
    Decay,
    Sustain,
    Release,
  };

  dsp::ProcessorParameter * attack_param_{};
  dsp::ProcessorParameter * decay_param_{};
  dsp::ProcessorParameter * sustain_param_{};
  dsp::ProcessorParameter * release_param_{};
  dsp::MidiPort *           midi_in_{};
  dsp::CVPort *             cv_out_{};
  MidiNoteTracker           tracker_;
  double                    sample_rate_{};
  Stage                     stage_{ Stage::Off };
  float                     level_{};
  int                       held_note_{ -1 };
};

} // namespace zrythm::plugins::modulators
