// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/modulators/modulator_engine.h"
#include "plugins/plugin_descriptor.h"

namespace zrythm::plugins::modulators
{

/**
 * @brief Math modulator engine.
 *
 * Combines the two CV inputs into one CV output according to the operation
 * parameter. An input without sources reads as zero. Results are clamped to
 * the CV range 0-1.
 */
class MathEngine final : public ModulatorEngine
{
public:
  static constexpr std::string_view kDescriptorId{ "zrythm.modulators.math" };
  static constexpr std::string_view kOperationParameterId{
    "/zrythm/modulators/math/operation"
  };
  static constexpr std::string_view kInputASymbol{ "a_in" };
  static constexpr std::string_view kInputBSymbol{ "b_in" };
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
  dsp::ProcessorParameter * operation_param_{};
  dsp::CVPort *             a_in_{};
  dsp::CVPort *             b_in_{};
  dsp::CVPort *             cv_out_{};
};

} // namespace zrythm::plugins::modulators
