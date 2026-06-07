// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/cv_port.h"
#include "dsp/midi_port.h"
#include "dsp/port.h"
#include "dsp/port_observer.h"
#include "utils/traits.h"

namespace zrythm::dsp
{

PortObserver::PortObserver (
  utils::IObjectRegistry &registry,
  const Port             &observed_port)
    : ProcessorBase (
        registry,
        observed_port.get_full_designation () + u8"/Observer"),
      observed_port_uuid_ (observed_port.get_uuid ())
{
  if (const auto * audio_port = qobject_cast<const AudioPort *> (&observed_port))
    typed_port_ = audio_port;
  else if (const auto * cv_port = qobject_cast<const CVPort *> (&observed_port))
    typed_port_ = cv_port;
  else if (
    const auto * midi_port = qobject_cast<const MidiPort *> (&observed_port))
    typed_port_ = midi_port;
  else
    throw std::runtime_error ("Unknown port type in PortObserver");
}

const Port &
PortObserver::observed_port () const
{
  return std::visit (
    [] (const auto &port) -> const Port & { return *port; }, typed_port_);
}

void
PortObserver::custom_prepare_for_processing (
  const graph::GraphNode * node,
  units::sample_rate_t     sample_rate,
  units::sample_u32_t      max_block_length)
{
  std::visit (
    [&] (const auto &port) {
      using T = utils::base_type<decltype (port)>;
      if constexpr (std::is_same_v<T, AudioPort> || std::is_same_v<T, CVPort>)
        {
          const auto ring_size =
            sample_rate.in<size_t> (units::sample_rate) * kAudioRingSeconds;
          const int num_channels = [&] {
            if constexpr (std::is_same_v<T, AudioPort>)
              return static_cast<int> (port->num_channels ());
            else
              return 1;
          }();
          audio_rings_.clear ();
          audio_rings_.reserve (num_channels);
          for (int ch = 0; ch < num_channels; ++ch)
            audio_rings_.push_back (
              std::make_unique<RingBuffer<float>> (ring_size));
        }
      else if constexpr (std::is_same_v<T, MidiPort>)
        {
          midi_ring_ =
            std::make_unique<RingBuffer<RealtimeMidiEvent>> (kMidiRingSize);
        }
    },
    typed_port_);
}

void
PortObserver::custom_release_resources ()
{
  audio_rings_.clear ();
  midi_ring_.reset ();
}

void
PortObserver::custom_process_block (
  dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  std::visit (
    [&] (const auto &port) {
      using T = utils::base_type<decltype (port)>;
      if constexpr (std::is_same_v<T, AudioPort>)
        process_audio (*port, time_nfo);
      else if constexpr (std::is_same_v<T, CVPort>)
        process_cv (*port, time_nfo);
      else if constexpr (std::is_same_v<T, MidiPort>)
        process_midi (*port);
    },
    typed_port_);
}

void
PortObserver::process_audio (
  const AudioPort             &port,
  dsp::graph::ProcessBlockInfo time_nfo) noexcept
{
  const auto &buf = port.buffers ();
  if (buf == nullptr)
    return;

  const int start = time_nfo.buffer_offset_.in<int> (units::samples);
  const int len = time_nfo.nframes_.in<int> (units::samples);
  if (len <= 0 || start + len > buf->getNumSamples ())
    return;

  const int num_ch =
    std::min (buf->getNumChannels (), static_cast<int> (audio_rings_.size ()));
  for (int ch = 0; ch < num_ch; ++ch)
    {
      if (audio_rings_[ch])
        {
          const auto * data = buf->getReadPointer (ch, start);
          audio_rings_[ch]->force_write_multiple (
            data, static_cast<size_t> (len));
        }
    }
}

void
PortObserver::process_cv (
  const CVPort                &port,
  dsp::graph::ProcessBlockInfo time_nfo) noexcept
{
  const int start = time_nfo.buffer_offset_.in<int> (units::samples);
  const int len = time_nfo.nframes_.in<int> (units::samples);
  if (len <= 0 || start + len > static_cast<int> (port.buf_.size ()))
    return;

  if (!audio_rings_.empty () && audio_rings_[0])
    {
      audio_rings_[0]->force_write_multiple (
        port.buf_.data () + start, static_cast<size_t> (len));
    }
}

void
PortObserver::process_midi (const MidiPort &port) noexcept
{
  if (midi_ring_)
    {
      for (const auto &view : port.buffer_)
        {
          // Skip large messages (e.g. SysEx) — only inline channel
          // messages are forwarded to the observer ring buffer.
          if (view.data ().size () > dsp::RealtimeMidiEvent::inline_capacity)
            continue;
          // TODO: eventually use a raw byte ring buffer so we can also
          // observe SysEx messages without constructing RealtimeMidiEvent.
          dsp::RealtimeMidiEvent ev;
          ev.set_inline_rt (view.data ());
          ev.time_ = view.time ();
          midi_ring_->write (ev);
        }
    }
}

}
