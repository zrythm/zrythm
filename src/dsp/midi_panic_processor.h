// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>

#include "dsp/processor_base.h"

namespace zrythm::dsp
{

/**
 * @brief A simple processor that outputs panic MIDI events when requested to.
 */
class MidiPanicProcessor final : public QObject, public dsp::ProcessorBase
{
  Q_OBJECT

public:
  MidiPanicProcessor (
    utils::IObjectRegistry &registry,
    QObject *               parent = nullptr);

  /**
   * @brief Request panic messages to be sent.
   *
   * This is realtime-safe and can be called from any thread.
   */
  void request_panic () { panic_.store (true); }

  // ============================================================================
  // ProcessorBase Interface
  // ============================================================================

  void custom_process_block (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept override;

  void custom_prepare_for_processing (
    const graph::GraphNode * node,
    units::sample_rate_t     sample_rate,
    units::sample_u32_t      max_block_length) override
  {
    midi_out_ = get_output_ports ().front ().get_object_as<dsp::MidiPort> ();
  }

  void custom_release_resources () override { midi_out_ = nullptr; }

  // ============================================================================

private:
  std::atomic_bool panic_;

  // Processing caches
  dsp::MidiPort * midi_out_{};
};
} // namespace zrythm::dsp
