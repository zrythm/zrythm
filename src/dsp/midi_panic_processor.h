// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>

#include "dsp/midi_port.h"
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
    ProcessorBaseDependencies dependencies,
    QObject *                 parent = nullptr)
      : QObject (parent), dsp::ProcessorBase (dependencies, u8"MIDI Panic")
  {
    add_output_port (dependencies.port_registry_.create_object<dsp::MidiPort> (
      u8"Panic MIDI out", dsp::PortFlow::Output));
  }

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
    EngineProcessTimeInfo  time_nfo,
    const dsp::ITransport &transport) noexcept override
  {
    const auto panic = panic_.exchange (false);
    if (!panic)
      return;

    // queue panic event
    midi_out_->midi_events_.queued_events_.panic_without_lock (
      time_nfo.local_offset_);
  }

  void custom_prepare_for_processing (
    sample_rate_t sample_rate,
    nframes_t     max_block_length) override
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
