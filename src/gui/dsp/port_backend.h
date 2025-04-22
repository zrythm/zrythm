// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "dsp/port_identifier.h"
#include "gui/dsp/midi_event.h"
#include "gui/dsp/rtaudio_device.h"
#include "gui/dsp/rtmidi_device.h"
#include "utils/concurrency.h"
#include "utils/dsp.h"
#include "utils/jack.h"
#include "utils/monotonic_time_provider.h"

#if HAVE_JACK
#  include "weakjack/weak_libjack.h"
#endif

#if HAVE_RTMIDI
#  include <rtmidi_c.h>
#endif

#if HAVE_RTAUDIO
#  include <rtaudio_c.h>
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
   * @brief Function used to approve or reject MIDI events on the given channel.
   *
   * Currently should be used to filter out events based on @ref
   * Channel.all_midi_channels_ and @ref Channel.midi_channels_.
   *
   * @param channel The channel to approve or reject (0-based).
   */
  using IsMidiChannelAcceptedFunc = std::function<bool (midi_byte_t channel)>;

  using PortDesignationProvider = std::function<std::string ()>;

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
    MidiEvents               &midi_events,
    FrameRange                range,
    IsMidiChannelAcceptedFunc approve_func)
  {
  }

  /**
   * Sends the port data to the backend, after the port is processed.
   */
  virtual void send_data (const float * buf, FrameRange range) { }
  virtual void send_data (const MidiEvents &midi_events, FrameRange range) { }

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

#if HAVE_JACK
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

  void sum_midi_data (
    MidiEvents               &midi_events,
    FrameRange                range,
    IsMidiChannelAcceptedFunc approve_func) override
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
            midi_byte_t channel = jack_ev.buffer[0] & 0xf;
            if (jack_ev.size == 3 && approve_func (channel))
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
  void send_data (const MidiEvents &midi_events, FrameRange range) override
  {
    if (jack_port_connected (port_) <= 0)
      {
        return;
      }

    void * buf = jack_port_get_buffer (port_, range.start_frame + range.nframes);
    midi_events.active_events_.copy_to_jack (
      range.start_frame, range.nframes, buf);
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

#if HAVE_RTMIDI
class RtMidiPortBackend : public PortBackend
{
public:
  RtMidiPortBackend () = default;

  void expose (
    const dsp::PortIdentifier &id,
    PortDesignationProvider    designation_provider) override
  {
    // FIXME: why do we not create devices here?
    // note: devices are created in ExtPort
    z_debug ("exposing {}", designation_provider ());
  }

  void unexpose () override { rtmidi_ins_.clear (); }

  bool is_exposed () const override { return !rtmidi_ins_.empty (); }

  void sum_midi_data (
    MidiEvents               &midi_events,
    FrameRange                range,
    IsMidiChannelAcceptedFunc approve_func) override
  {
    if (range.start_frame == 0)
      {
        prepare_rtmidi_data (range.nframes);
      }

    for (auto &dev : rtmidi_ins_)
      {
        for (auto &ev : dev->events_)
          {
            if (
              ev.time_ >= range.start_frame
              && ev.time_ < range.start_frame + range.nframes)
              {
                const midi_byte_t channel = ev.raw_buffer_[0] & 0xf;
                if (approve_func (channel))
                  {
                    midi_events.active_events_.add_event_from_buf (
                      ev.time_, ev.raw_buffer_.data (), 3);
                  }
              }
          }
      }
  }

  void clear_backend_buffer (dsp::PortType type, nframes_t nframes) override
  {
    // not needed for RtMidi (?)
  }

  void
  set_devices (const std::vector<std::shared_ptr<RtMidiDevice>> &rtmidi_ins)
  {
    rtmidi_ins_ = rtmidi_ins;
  }

  const auto &get_time_provider () const { return time_provider_; }

  auto get_last_dequeue_time_usecs () const { return last_midi_dequeue_; }

private:
  /**
   * Dequeue the MIDI events from the ring buffers into @ref
   * RtMidiDevice.events.
   */
  void prepare_rtmidi_data (nframes_t total_frames)
  {

    auto cur_time = time_provider_.get_monotonic_time_usecs ();
    for (auto &dev : rtmidi_ins_)
      {
        /* clear the events */
        dev->events_.clear ();

        SemaphoreRAII<> sem_raii (dev->midi_ring_sem_);
        MidiEvent       ev;
        while (dev->midi_ring_.read (ev))
          {
            /* calculate event timestamp */
            qint64 length = cur_time - last_midi_dequeue_;
            auto   ev_time =
              (midi_time_t) (((double) ev.systime_ / (double) length)
                             * (double) total_frames);

            if (ev_time >= total_frames)
              {
                z_warning (
                  "event with invalid time {} received. the maximum allowed time "
                  "is {}. setting it to {}...",
                  ev_time, total_frames - 1, total_frames - 1);
                ev_time = (midi_byte_t) (total_frames - 1);
              }

            dev->events_.add_event_from_buf (
              ev_time, ev.raw_buffer_.data (), ev.raw_buffer_sz_);
          }
      }
    last_midi_dequeue_ = cur_time;
  }

private:
  /**
   * RtMidi pointers for input ports.
   *
   * Each RtMidi port represents a device, and this Port can be connected to
   * multiple devices.
   */
  std::vector<std::shared_ptr<RtMidiDevice>> rtmidi_ins_;

  /** RtMidi pointers for output ports (unimplemented). */
  // std::vector<std::shared_ptr<RtMidiDevice>> rtmidi_outs_;

  utils::QElapsedTimeProvider time_provider_;

  /**
   * Last timestamp we finished dequeueing MIDI events at.
   */
  utils::MonotonicTime last_midi_dequeue_{};
};
#endif

#if HAVE_RTAUDIO
/**
 * @brief
 *
 * This is only used on Stereo L/R audio ports associated with channels.
 */
class RtAudioPortBackend : public PortBackend
{
public:
  struct RtAudioPortInfo
  {
    bool is_input_;
    int  rtaudio_id_;
    int  rtaudio_channel_idx_;
    RtAudioPortInfo (bool is_input, int rtaudio_id, int rtaudio_channel_idx)
        : is_input_ (is_input), rtaudio_id_ (rtaudio_id),
          rtaudio_channel_idx_ (rtaudio_channel_idx)
    {
    }
  };

  /**
   * @brief Function that fills in RtAudioPortInfo for this L or R channel port.
   *
   * Corresponds to @ref Channel.ext_stereo_l_ins_ (or r_ins_).
   */
  using RtAudioPortInfoProvider =
    std::function<void (std::vector<RtAudioPortInfo> &)>;

  /**
   * @brief Function to return whether all stereo L or R inputs are enabled on
   * this channel.
   */
  using AllStereoInsFlagProvider = std::function<bool ()>;

  RtAudioPortBackend (
    RtAudioPortInfoProvider  nfo_provider,
    AllStereoInsFlagProvider all_stereo_ins_flag_provider)
      : nfo_provider_ (std::move (nfo_provider)),
        all_stereo_ins_flag_provider_ (std::move (all_stereo_ins_flag_provider))
  {
  }

  void clear_backend_buffer (dsp::PortType type, nframes_t nframes) override
  {
    // not needed for RtAudio
  }

  void sum_data (float * buf, FrameRange range) override
  {
    if (range.start_frame == 0)
      {
        prepare_rtaudio_data (range.nframes);
      }

    for (auto &dev : rtaudio_ins_)
      {
        utils::float_ranges::add2 (
          &buf[range.start_frame], &dev->audio_buf_.data ()[range.start_frame],
          range.nframes);
      }
  }

  void expose (
    const dsp::PortIdentifier &id,
    PortDesignationProvider    designation_provider) override
  {
    // only inputs supported at the moment
    if (id.is_input ())
      {
        if (!all_stereo_ins_flag_provider_ ())
          {
            std::vector<RtAudioPortInfo> ext_ports;
            nfo_provider_ (ext_ports);
            for (const auto &ext_port : ext_ports)
              {
                auto dev = std::make_shared<RtAudioDevice> (
                  ext_port.is_input_, ext_port.rtaudio_id_,
                  ext_port.rtaudio_channel_idx_);
                dev->open (true);
                rtaudio_ins_.emplace_back (std::move (dev));
              }
          }
      }
    z_debug ("exposing {}", designation_provider ());
  }

  void unexpose () override { rtaudio_ins_.clear (); }

  bool is_exposed () const override { return !rtaudio_ins_.empty (); }

  void
  set_devices (const std::vector<std::shared_ptr<RtAudioDevice>> &rtaudio_ins)
  {
    rtaudio_ins_ = rtaudio_ins;
  }

private:
  /**
   * Dequeue the audio data from the ring buffers into @ref RtAudioDevice.buf.
   */
  void prepare_rtaudio_data (nframes_t total_frames)
  {
    for (auto &dev : rtaudio_ins_)
      {
        SemaphoreRAII<> sem_raii (dev->audio_ring_sem_);

        /* either copy the data from the ring buffer or fill with 0 */
        if (
          !dev->audio_ring_.read_multiple (dev->audio_buf_.data (), total_frames))
          {
            utils::float_ranges::fill (
              dev->audio_buf_.data (), utils::math::ALMOST_SILENCE,
              total_frames);
          }
      }
  }

private:
  /**
   * RtAudio pointers for input ports.
   *
   * Each port can have multiple RtAudio devices.
   */
  std::vector<std::shared_ptr<RtAudioDevice>> rtaudio_ins_;

  RtAudioPortInfoProvider  nfo_provider_;
  AllStereoInsFlagProvider all_stereo_ins_flag_provider_;
};
#endif
