// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "dsp/midi_event.h"
#include "dsp/port_identifier.h"
#include "utils/concurrency.h"
#include "utils/dsp.h"
#include "utils/jack.h"
#include "utils/monotonic_time_provider.h"

#ifdef HAVE_JACK
#  include "weakjack/weak_libjack.h"
#endif

class PortBackend
{
public:
  virtual ~PortBackend () = default;

  /**
   * @brief Frame range to operate on (on both the port's buffers and the
   * backend's buffers).
   */
  struct FrameRange
  {
    nframes_t start_frame = 0;
    nframes_t nframes = 0;
  };

  /**
   * @brief Function used to filter MIDI events.
   */
  using MidiEventFilter = std::function<bool (const dsp::MidiEvent &)>;

  using PortDesignationProvider = std::function<utils::Utf8String ()>;

  /**
   * Sums the inputs coming in from the backend, before the port is processed.
   *
   * @param buf The buffer to sum into.
   */
  virtual void sum_audio_data (float * buf, FrameRange range) { }

  /**
   * @brief
   *
   * @note MIDI timings are assumed to be at the correct positions in the
   * current cycle (ie, already added the start_frames in this cycle).
   */
  virtual void sum_midi_data (
    dsp::MidiEvents &midi_events,
    FrameRange       range,
    MidiEventFilter  filter_func = [] (const auto &) { return true; })
  {
  }

  /**
   * Sends the port data to the backend, after the port is processed.
   */
  virtual void send_data (const float * buf, FrameRange range) { }
  virtual void send_data (const dsp::MidiEvents &midi_events, FrameRange range)
  {
  }

  /**
   * Exposes the port to the backend, if not already exposed. If already
   * exposed, updates any metadata if necessary.
   */
  virtual void expose (
    const dsp::PortIdentifier &id,
    PortDesignationProvider    designation_provider) = 0;

  /**
   * @brief Unexposes the port from the backend (if exposed).
   */
  virtual void unexpose () = 0;

  virtual void clear_backend_buffer (dsp::PortType type, nframes_t nframes) = 0;

  virtual bool is_exposed () const = 0;
};

#ifdef HAVE_JACK
class JackPortBackend : public PortBackend
{
public:
  JackPortBackend (jack_client_t &client) : client_ (client) { }
  ~JackPortBackend () override
  {
    {
      if (is_exposed ())
        {
          unexpose ();
        }
    }
  }

  void sum_audio_data (float * buf, FrameRange range) override
  {
    const auto * in = reinterpret_cast<const float *> (
      jack_port_get_buffer (port_, range.start_frame + range.nframes));

    utils::float_ranges::add2 (
      &buf[range.start_frame], &in[range.start_frame], range.nframes);
  }

  /**
   * Writes the events to the given JACK buffer.
   */
  void copy_midi_event_vector_to_jack (
    const dsp::MidiEventVector &src,
    const nframes_t             local_start_frames,
    const nframes_t             nframes,
    void *                      buff) const
  {
    /*jack_midi_clear_buffer (buff);*/

    src.foreach_event ([&] (const auto &ev) {
      if (
        ev.time_ < local_start_frames
        || ev.time_ >= local_start_frames + nframes)
        {
          return;
        }

      std::array<jack_midi_data_t, 3> midi_data{};
      std::copy_n (
        midi_data.data (), ev.raw_buffer_sz_, (jack_midi_data_t *) buff);
      jack_midi_event_write (
        buff, ev.time_, midi_data.data (), ev.raw_buffer_sz_);
#  if 0
      z_info (
        "wrote MIDI event to JACK MIDI out at %d", ev.time);
#  endif
    });
  }

  void sum_midi_data (
    dsp::MidiEvents &midi_events,
    FrameRange       range,
    MidiEventFilter  filter_func) override
  {
    const auto start_frame = range.start_frame;
    const auto nframes = range.nframes;
    void *     port_buf = jack_port_get_buffer (port_, nframes);
    uint32_t   num_events = jack_midi_get_event_count (port_buf);

    jack_midi_event_t jack_ev;
    for (unsigned i = 0; i < num_events; i++)
      {
        jack_midi_event_get (&jack_ev, port_buf, i);

        if (jack_ev.time >= start_frame && jack_ev.time < start_frame + nframes)
          {
            if (
              jack_ev.size == 3
              && filter_func (dsp::MidiEvent{
                jack_ev.buffer[0], jack_ev.buffer[1], jack_ev.buffer[2],
                jack_ev.time }))
              {
                /*z_debug ("received event at {}", jack_ev.time);*/
                midi_events.active_events_.add_event_from_buf (
                  jack_ev.time, jack_ev.buffer, (int) jack_ev.size);
              }
            else
              {
                /* different channel */
                /*z_debug ("received event on different channel");*/
              }
          }
      }
  }

  void send_data (const float * buf, FrameRange range) override
  {
    if (jack_port_connected (port_) <= 0)
      {
        return;
      }

    auto * out = reinterpret_cast<float *> (
      jack_port_get_buffer (port_, range.start_frame + range.nframes));

    utils::float_ranges::copy (
      &out[range.start_frame], &buf[range.start_frame], range.nframes);
  }
  void send_data (const dsp::MidiEvents &midi_events, FrameRange range) override
  {
    if (jack_port_connected (port_) <= 0)
      {
        return;
      }

    void * buf = jack_port_get_buffer (port_, range.start_frame + range.nframes);
    copy_midi_event_vector_to_jack (
      midi_events.active_events_, range.start_frame, range.nframes, buf);
  }

  void expose (
    const dsp::PortIdentifier &id,
    PortDesignationProvider    designation_provider) override
  {
    enum JackPortFlags flags
    {
    };
    /* these are reversed */
    if (id.is_input ())
      flags = JackPortIsOutput;
    else if (id.is_output ())
      flags = JackPortIsInput;
    else
      {
        z_return_if_reached ();
      }

    const char * type = get_jack_type (id.type_);
    if (type == nullptr)
      z_return_if_reached ();

    auto label = designation_provider ();
    z_info ("exposing port {} to JACK", label);
    if (port_ == nullptr)
      {
        port_ = jack_port_register (&client_, label.c_str (), type, flags, 0);
        z_return_if_fail (port_);
      }
    else
      {
        jack_port_rename (&client_, port_, label.c_str ());
      }
  }

  void unexpose () override
  {
    if (!is_exposed ())
      return;

    int ret = jack_port_unregister (&client_, port_);
    port_ = nullptr;
    if (ret != 0)
      {
        auto jack_error = utils::jack::get_error_message ((jack_status_t) ret);
        z_warning ("JACK port unregister error: {}", jack_error);
      }
  }

  bool is_exposed () const override { return port_ != nullptr; }

  void clear_backend_buffer (dsp::PortType type, nframes_t nframes) override
  {
    z_return_if_fail (port_ != nullptr);
    void * buf = jack_port_get_buffer (port_, nframes);
    z_return_if_fail (buf);
    if (type == dsp::PortType::Audio)
      {
        auto * fbuf = static_cast<float *> (buf);
        utils::float_ranges::fill (
          &fbuf[0], utils::math::ALMOST_SILENCE, nframes);
      }
    else if (type == dsp::PortType::Event)
      {
        jack_midi_clear_buffer (buf);
      }
  }

  jack_port_t * get_jack_port () const { return port_; }

  /**
   * Returns the JACK port type string.
   */
  static const char * get_jack_type (dsp::PortType type)
  {
    switch (type)
      {
      case dsp::PortType::Audio:
        return JACK_DEFAULT_AUDIO_TYPE;
        break;
      case dsp::PortType::Event:
        return JACK_DEFAULT_MIDI_TYPE;
        break;
      default:
        return nullptr;
      }
  }

private:
  jack_port_t *  port_{ nullptr };
  jack_client_t &client_;
};
#endif
