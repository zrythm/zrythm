// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/modulators/modulator_engine.h"
#include "plugins/plugin_descriptor.h"

namespace zrythm::plugins::modulators
{

/**
 * @brief LFO modulator engine.
 *
 * Emits a periodic unipolar (0-1) waveform. Free-running mode advances at the
 * rate parameter's frequency. Tempo-synced mode derives its rate from the
 * tempo at the current transport position and the selected note division,
 * and its phase from the timeline position, so playback from any point,
 * looping and offline rendering produce the same waveform.
 */
class LfoEngine final : public ModulatorEngine
{
public:
  static constexpr std::string_view kDescriptorId{ "zrythm.modulators.lfo" };
  static constexpr std::string_view kRateParameterId{
    "/zrythm/modulators/lfo/rate"
  };
  static constexpr std::string_view kSyncParameterId{
    "/zrythm/modulators/lfo/sync"
  };
  static constexpr std::string_view kDivisionParameterId{
    "/zrythm/modulators/lfo/division"
  };
  static constexpr std::string_view kWaveformParameterId{
    "/zrythm/modulators/lfo/waveform"
  };
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
  dsp::ProcessorParameter * rate_param_{};
  dsp::ProcessorParameter * sync_param_{};
  dsp::ProcessorParameter * division_param_{};
  dsp::ProcessorParameter * waveform_param_{};
  dsp::CVPort *             cv_out_{};
  double                    sample_rate_{};
  /** Cycle phase (0-1). Tempo-synced mode recomputes it from the timeline
   * position at each block start; free-running mode advances it across
   * blocks. */
  double phase_{};
};

} // namespace zrythm::plugins::modulators
