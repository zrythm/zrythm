// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "dsp/graph.h"
#include "dsp/parameter.h"
#include "dsp/speaker_arrangement.h"

namespace zrythm::plugins::modulators
{

class Modulator;
class ModulatorRuntimeContext;

/** A parameter owned by a native modulator engine. */
struct ModulatorParameterSpec final
{
  utils::Utf8String   unique_id_;
  utils::Utf8String   label_;
  dsp::ParameterRange range_;
};

/** An input or output port owned by a native modulator engine. */
struct ModulatorPortSpec final
{
  utils::Utf8String       symbol_;
  utils::Utf8String       label_;
  dsp::PortType           type_{};
  dsp::SpeakerArrangement audio_arrangement_{};
};

/** Static port and parameter topology of a native modulator engine. */
struct ModulatorLayout final
{
  std::vector<ModulatorPortSpec>      input_ports_;
  std::vector<ModulatorPortSpec>      output_ports_;
  std::vector<ModulatorParameterSpec> parameters_;
};

/**
 * @brief Processing behavior of one native modulator node.
 *
 * The Modulator host creates ports and parameters from layout(), resolves the
 * restored objects into a runtime context at prepare time, and forwards
 * processing to the engine. Engines hold no serialized state.
 */
class ModulatorEngine
{
public:
  virtual ~ModulatorEngine () = default;

  /** Returns the engine's static port and parameter topology. */
  [[nodiscard]] virtual ModulatorLayout layout () const = 0;

  /** Caches raw pointers to the prepared ports and parameters. */
  virtual void prepare (const ModulatorRuntimeContext &context) noexcept = 0;

  virtual void release_resources () noexcept = 0;

  virtual void process (
    dsp::graph::ProcessBlockInfo time_info,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept [[clang::nonblocking]] = 0;
};

} // namespace zrythm::plugins::modulators
