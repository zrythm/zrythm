// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/modulators/modulator_engine.h"
#include "plugins/plugin_descriptor.h"

namespace zrythm::plugins::modulators
{

/**
 * @brief Envelope follower modulator engine.
 *
 * Emits a 0-1 signal that tracks the loudness of the incoming audio. The
 * envelope rises towards the instantaneous peak at the attack rate and falls
 * at the release rate, both as exponential smoothing.
 */
class EnvelopeFollowerEngine final : public ModulatorEngine
{
public:
  static constexpr std::string_view kDescriptorId{
    "zrythm.modulators.envelope_follower"
  };
  static constexpr std::string_view kAttackParameterId{
    "/zrythm/modulators/envelope_follower/attack"
  };
  static constexpr std::string_view kReleaseParameterId{
    "/zrythm/modulators/envelope_follower/release"
  };
  static constexpr std::string_view kInputSymbol{ "audio_in" };
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
  dsp::ProcessorParameter * attack_param_{};
  dsp::ProcessorParameter * release_param_{};
  dsp::AudioPort *          audio_in_{};
  dsp::CVPort *             cv_out_{};
  double                    sample_rate_{};
  float                     envelope_{};
};

} // namespace zrythm::plugins::modulators
