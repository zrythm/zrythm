// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/modulators/modulator_engine.h"
#include "plugins/plugin_descriptor.h"

namespace zrythm::plugins::modulators
{

/**
 * @brief Macro modulator engine.
 *
 * With a connected CV input, the output is the summed input multiplied by the
 * macro value. Without one, the output is the macro value itself.
 */
class MacroEngine final : public ModulatorEngine
{
public:
  static constexpr std::string_view kDescriptorId{ "zrythm.modulators.macro" };
  static constexpr std::string_view kParameterId{
    "/zrythm/modulators/macro/value"
  };
  static constexpr std::string_view kInputSymbol{ "cv_in" };
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
  dsp::ProcessorParameter * macro_param_{};
  dsp::CVPort *             cv_in_{};
  dsp::CVPort *             cv_out_{};
};

} // namespace zrythm::plugins::modulators
