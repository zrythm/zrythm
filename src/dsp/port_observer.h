// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <variant>

#include "dsp/processor_base.h"
#include "utils/ring_buffer.h"

#include <QObject>

#include "midi_event.h"

namespace zrythm::dsp
{

class Port;
class AudioPort;
class MidiPort;
class CVPort;

using ObservedPortPtr =
  std::variant<const AudioPort *, const CVPort *, const MidiPort *>;

/**
 * @brief Pure data-capture graph node that observes a port's output.
 *
 * Copies raw audio samples to per-channel RingBuffer<float> and raw MIDI
 * events to RingBuffer<RealtimeMidiEvent>. No DSP, no metering — all signal
 * processing stays on the UI side.
 *
 * RT thread writes to ring buffers via custom_process_block().
 * UI-side drain timer consuming-reads into per-requester token caches.
 *
 * @note Only made a QObject so that GraphExport can get its class name.
 */
class PortObserver : public QObject, public ProcessorBase
{
  Q_OBJECT
  Q_DISABLE_COPY_MOVE (PortObserver)
public:
  static constexpr size_t kAudioRingSeconds = 5;
  static constexpr size_t kMidiRingSize = 8192;

  PortObserver (utils::IObjectRegistry &registry, const Port &observed_port);

  PortUuid    observed_port_uuid () const { return observed_port_uuid_; }
  const Port &observed_port () const;

  int num_channels () const { return static_cast<int> (audio_rings_.size ()); }

  // --- Audio ring buffers (RT writes, drain timer reads) ---
  RingBuffer<float> &audio_ring (int ch)
  {
    assert (ch >= 0 && ch < static_cast<int> (audio_rings_.size ()));
    assert (audio_rings_[ch] != nullptr);
    return *audio_rings_[ch];
  }
  const RingBuffer<float> &audio_ring (int ch) const
  {
    assert (ch >= 0 && ch < static_cast<int> (audio_rings_.size ()));
    assert (audio_rings_[ch] != nullptr);
    return *audio_rings_[ch];
  }

  // --- MIDI ring buffer (RT writes, drain timer reads) ---
  RingBuffer<RealtimeMidiEvent> &midi_ring ()
  {
    assert (midi_ring_ != nullptr);
    return *midi_ring_;
  }
  const RingBuffer<RealtimeMidiEvent> &midi_ring () const
  {
    assert (midi_ring_ != nullptr);
    return *midi_ring_;
  }

  bool has_audio_rings () const { return !audio_rings_.empty (); }
  bool has_midi_ring () const { return midi_ring_ != nullptr; }

private:
  void custom_process_block (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept override;

  void custom_prepare_for_processing (
    const graph::GraphNode * node,
    units::sample_rate_t     sample_rate,
    units::sample_u32_t      max_block_length) override;

  void custom_release_resources () override;

  void process_audio (
    const AudioPort             &port,
    dsp::graph::ProcessBlockInfo time_nfo) noexcept [[clang::nonblocking]];
  void
  process_cv (const CVPort &port, dsp::graph::ProcessBlockInfo time_nfo) noexcept
    [[clang::nonblocking]];
  void process_midi (const MidiPort &port) noexcept [[clang::nonblocking]];

  PortUuid        observed_port_uuid_;
  ObservedPortPtr typed_port_;

  std::vector<std::unique_ptr<RingBuffer<float>>> audio_rings_;
  std::unique_ptr<RingBuffer<RealtimeMidiEvent>>  midi_ring_;
};

}
