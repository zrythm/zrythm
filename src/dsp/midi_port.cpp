// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/channel_track.h"
#include "dsp/engine.h"
#include "dsp/engine_jack.h"
#include "dsp/midi_port.h"
#include "dsp/piano_roll_track.h"
#include "dsp/processable_track.h"
#include "dsp/recordable_track.h"
#include "dsp/rtmidi_device.h"
#include "dsp/track_processor.h"
#include "gui/backend/event_manager.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/gtest_wrapper.h"
#include "utils/midi.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

MidiPort::MidiPort (std::string label, PortFlow flow)
    : Port (label, PortType::Event, flow, 0.f, 1.f, 0.f)
{
}

MidiPort::~MidiPort ()
{
#if HAVE_RTMIDI
  for (auto &dev : rtmidi_ins_)
    {
      dev->close ();
    }
#endif
}

void
MidiPort::init_after_cloning (const MidiPort &original)
{
  Port::copy_members_from (original);
}

void
MidiPort::allocate_bufs ()
{
  midi_ring_ = std::make_unique<RingBuffer<MidiEvent>> (11);
}

void
MidiPort::clear_buffer (AudioEngine &engine)
{
  midi_events_.active_events_.clear ();
  // midi_events_.queued_events_.clear ();
}

#if HAVE_JACK
void
MidiPort::receive_midi_events_from_jack (
  const nframes_t start_frame,
  const nframes_t nframes)
{
  if (
    this->internal_type_ != Port::InternalType::JackPort
    || this->id_.type_ != PortType::Event)
    return;

  void *   port_buf = jack_port_get_buffer (JACK_PORT_T (this->data_), nframes);
  uint32_t num_events = jack_midi_get_event_count (port_buf);

  jack_midi_event_t jack_ev;
  for (unsigned i = 0; i < num_events; i++)
    {
      jack_midi_event_get (&jack_ev, port_buf, i);

      if (jack_ev.time >= start_frame && jack_ev.time < start_frame + nframes)
        {
          midi_byte_t channel = jack_ev.buffer[0] & 0xf;
          Track *     track = this->get_track (false);
          if (
            this->id_.owner_type_ == PortIdentifier::OwnerType::TrackProcessor
            && !track)
            {
              z_return_if_reached ();
            }

          auto channel_track = dynamic_cast<ChannelTrack *> (track);
          if (
            this->id_.owner_type_ == PortIdentifier::OwnerType::TrackProcessor
            && channel_track && (track->is_midi () || track->is_instrument ())
            && !channel_track->channel_->all_midi_channels_
            && !channel_track->channel_->midi_channels_[channel])
            {
              /* different channel */
              /*z_debug ("received event on different channel");*/
            }
          else if (jack_ev.size == 3)
            {
              /*z_debug ("received event at {}", jack_ev.time);*/
              midi_events_.active_events_.add_event_from_buf (
                jack_ev.time, jack_ev.buffer, (int) jack_ev.size);
            }
        }
    }

#  if 0
  if (midi_events_has_any (this->midi_events_, F_NOT_QUEUED))
    {
      char designation[600];
      this->get_full_designation ( designation);
      z_debug ("JACK MIDI ({}): have {} events", designation, num_events);
      midi_events_print (this->midi_events_, F_NOT_QUEUED);
    }
#  endif
}

void
MidiPort::send_midi_events_to_jack (
  const nframes_t start_frames,
  const nframes_t nframes)
{
  if (
    internal_type_ != Port::InternalType::JackPort
    || id_.type_ != PortType::Event)
    return;

  jack_port_t * jport = JACK_PORT_T (data_);

  if (jack_port_connected (jport) <= 0)
    {
      return;
    }

  void * buf = jack_port_get_buffer (jport, AUDIO_ENGINE->nframes_);
  midi_events_.active_events_.copy_to_jack (start_frames, nframes, buf);
}
#endif // HAVE_JACK

#if HAVE_RTMIDI
void
MidiPort::sum_data_from_rtmidi (
  const nframes_t start_frame,
  const nframes_t nframes)
{
  z_return_if_fail (
    // is_input() &&
    midi_backend_is_rtmidi (AUDIO_ENGINE->midi_backend_));

  Track * track = get_track (false);
  auto *  channel_track = dynamic_cast<ChannelTrack *> (track);
  for (auto &dev : rtmidi_ins_)
    {
      for (auto &ev : dev->events_)
        {
          if (ev.time_ >= start_frame && ev.time_ < start_frame + nframes)
            {
              midi_byte_t channel = ev.raw_buffer_[0] & 0xf;
              if (
                id_.owner_type_ == PortIdentifier::OwnerType::TrackProcessor
                && (track == nullptr)) [[unlikely]]
                {
                  z_return_if_reached ();
                }

              if (
                id_.owner_type_ == PortIdentifier::OwnerType::TrackProcessor
                && track->has_piano_roll () && channel_track
                && !channel_track->channel_->all_midi_channels_
                && !channel_track->channel_->midi_channels_.at (channel))
                {
                  /* different channel */
                }
              else
                {
                  midi_events_.active_events_.add_event_from_buf (
                    ev.time_, ev.raw_buffer_.data (), 3);
                }
            }
        }
    }

#  if 0
  if (DEBUGGING &&
      this->midi_events_->num_events > 0)
    {
      MidiEvent * ev =
        &this->midi_events_->events[0];
      char designation[600];
      port_get_full_designation (
        this, designation);
      z_info (
        "RtMidi (%s): have %d events\n"
        "first event is: [{}] %hhx %hhx %hhx",
        designation, this->midi_events_->num_events,
        ev->time, ev->raw_buffer[0],
        ev->raw_buffer[1], ev->raw_buffer[2]);
    }
#  endif
}

void
MidiPort::prepare_rtmidi_events ()
{
  z_return_if_fail (
    // is_input() &&
    midi_backend_is_rtmidi (AUDIO_ENGINE->midi_backend_));

  gint64 cur_time = g_get_monotonic_time ();
  for (auto &dev : rtmidi_ins_)
    {
      /* clear the events */
      dev->events_.clear ();

      SemaphoreRAII<> sem_raii (dev->midi_ring_sem_);
      MidiEvent       ev;
      while (dev->midi_ring_.read (ev))
        {
          /* calculate event timestamp */
          gint64 length = cur_time - last_midi_dequeue_;
          auto   ev_time =
            (midi_time_t) (((double) ev.systime_ / (double) length)
                           * (double) AUDIO_ENGINE->block_length_);

          if (ev_time >= AUDIO_ENGINE->block_length_)
            {
              z_warning (
                "event with invalid time {} received. the maximum allowed time "
                "is {}. setting it to {}...",
                ev_time, AUDIO_ENGINE->block_length_ - 1,
                AUDIO_ENGINE->block_length_ - 1);
              ev_time = (midi_byte_t) (AUDIO_ENGINE->block_length_ - 1);
            }

          dev->events_.add_event_from_buf (
            ev_time, ev.raw_buffer_.data (), ev.raw_buffer_sz_);
        }
    }
  last_midi_dequeue_ = cur_time;
}

void
MidiPort::expose_to_rtmidi (bool expose)
{
  if (expose)
    {
#  if 0

      if (self->id_.flow_ == PortFlow::Input)
        {
          self->rtmidi_in =
            rtmidi_in_create (
#    ifdef _WIN32
              RTMIDI_API_WINDOWS_MM,
#    elif defined(__APPLE__)
              RTMIDI_API_MACOSX_CORE,
#    else
              RTMIDI_API_LINUX_ALSA,
#    endif
              "Zrythm",
              AUDIO_ENGINE->midi_buf_size);

          /* don't ignore any messages */
          rtmidi_in_ignore_types (
            self->rtmidi_in, 0, 0, 0);

          rtmidi_open_port (
            self->rtmidi_in, 1, lbl);
        }
#  endif
      z_debug ("exposing {}", get_full_designation ());
    }
  else
    {
#  if 0
      if (self->id_.flow_ == PortFlow::Input &&
          self->rtmidi_in)
        {
          rtmidi_close_port (self->rtmidi_in);
          self->rtmidi_in = NULL;
        }
#  endif
      z_debug ("unexposing {}", get_full_designation ());
    }
  exposed_to_backend_ = expose;
}
#endif // HAVE_RTMIDI

void
MidiPort::process (const EngineProcessTimeInfo time_nfo, const bool noroll)
{
  if (noroll)
    return;

  midi_events_.dequeue (time_nfo.local_offset_, time_nfo.nframes_);

  const auto id = id_;
  const auto owner_type = id.owner_type_;
  Track *    track = nullptr;
  if (
    owner_type == PortIdentifier::OwnerType::TrackProcessor
    || owner_type == PortIdentifier::OwnerType::Track
    || owner_type == PortIdentifier::OwnerType::Channel ||
    /* if track/channel fader */
    (owner_type == PortIdentifier::OwnerType::Fader
     && (ENUM_BITSET_TEST (PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::Prefader) || ENUM_BITSET_TEST (PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::Postfader)))
    || (owner_type == PortIdentifier::OwnerType::Plugin && id_.plugin_id_.slot_type_ == PluginSlotType::Instrument))
    {
      if (ZRYTHM_TESTING)
        track = get_track (true);
      else
        track = track_;
      z_return_if_fail (track);
    }
  const auto * const processable_track =
    dynamic_cast<const ProcessableTrack *> (track);
  const auto * const channel_track = dynamic_cast<const ChannelTrack *> (track);
  const auto * const recordable_track =
    dynamic_cast<const RecordableTrack *> (track);
  auto &events = midi_events_.active_events_;

  /* if piano roll keys, add the notes to the piano roll "current notes" (to
   * show pressed keys in the UI) */
  if (
    G_UNLIKELY (
      owner_type == PortIdentifier::OwnerType::TrackProcessor
      && this == processable_track->processor_->midi_out_.get ()
      && events.has_any () && CLIP_EDITOR->has_region_
      && CLIP_EDITOR->region_id_.track_name_hash_ == track->get_name_hash ()))
    {
      bool events_processed = false;
      for (auto &ev : events)
        {
          auto &buf = ev.raw_buffer_;
          if (midi_is_note_on (buf.data ()))
            {
              PIANO_ROLL->add_current_note (midi_get_note_number (buf.data ()));
              events_processed = true;
            }
          else if (midi_is_note_off (buf.data ()))
            {
              PIANO_ROLL->remove_current_note (
                midi_get_note_number (buf.data ()));
              events_processed = true;
            }
          else if (midi_is_all_notes_off (buf.data ()))
            {
              PIANO_ROLL->current_notes_.clear ();
              events_processed = true;
            }
        }
      if (events_processed)
        {
          EVENTS_PUSH (EventType::ET_PIANO_ROLL_KEY_ON_OFF, nullptr);
        }
    }

  /* only consider incoming external data if armed for recording (if the port is
   * owner by a track), otherwise always consider incoming external data */
  if ((owner_type != PortIdentifier::OwnerType::TrackProcessor || (owner_type == PortIdentifier::OwnerType::TrackProcessor && (recordable_track != nullptr) && recordable_track->get_recording())) && id.is_input())
    {
      switch (AUDIO_ENGINE->midi_backend_)
        {
#if HAVE_JACK
        case MidiBackend::MIDI_BACKEND_JACK:
          receive_midi_events_from_jack (
            time_nfo.local_offset_, time_nfo.nframes_);
          break;
#endif
#if HAVE_RTMIDI
        case MidiBackend::MIDI_BACKEND_ALSA_RTMIDI:
        case MidiBackend::MIDI_BACKEND_JACK_RTMIDI:
        case MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI:
        case MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI:
#  if HAVE_RTMIDI_6
        case MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI:
#  endif
          sum_data_from_rtmidi (time_nfo.local_offset_, time_nfo.nframes_);
          break;
#endif
        default:
          break;
        }
    }

  /* set midi capture if hardware */
  if (owner_type == PortIdentifier::OwnerType::HardwareProcessor)
    {
      if (events.has_any ())
        {
          AUDIO_ENGINE->trigger_midi_activity_.store (true);

          /* queue playback if recording and we should record on MIDI input */
          if (
            TRANSPORT->is_recording () && TRANSPORT->is_paused ()
            && TRANSPORT->start_playback_on_midi_input_)
            {
              EVENTS_PUSH (EventType::ET_TRANSPORT_ROLL_REQUIRED, nullptr);
            }

          /* capture cc if capturing */
          if (AUDIO_ENGINE->capture_cc_.load ()) [[unlikely]]
            {
              auto last_event = events.back ();
              AUDIO_ENGINE->last_cc_captured_ = last_event.raw_buffer_;
            }

          /* send cc to mapped ports */
          for (auto &ev : events)
            {
              MIDI_MAPPINGS->apply (ev.raw_buffer_.data ());
            }
        }
    }

  /* handle MIDI clock */
  if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::MidiClock)
    && is_output ())
    {
      /* continue or start */
      bool start =
        TRANSPORT->is_rolling () && !AUDIO_ENGINE->pos_nfo_before_.is_rolling_;
      if (start)
        {
          uint8_t start_msg = MIDI_CLOCK_CONTINUE;
          if (PLAYHEAD.frames_ == 0)
            {
              start_msg = MIDI_CLOCK_START;
            }
          events.add_raw (&start_msg, 1, 0);
        }
      else if (
        !TRANSPORT->is_rolling () && AUDIO_ENGINE->pos_nfo_before_.is_rolling_)
        {
          uint8_t stop_msg = MIDI_CLOCK_STOP;
          events.add_raw (&stop_msg, 1, 0);
        }

      /* song position */
      int32_t sixteenth_within_song = PLAYHEAD.get_total_sixteenths (false);
      if (
        AUDIO_ENGINE->pos_nfo_at_end_.sixteenth_within_song_
          != AUDIO_ENGINE->pos_nfo_current_.sixteenth_within_song_
        || start)
        {
          /* TODO interpolate */
          events.add_song_pos (sixteenth_within_song, 0);
        }

      /* clock beat */
      if (
        AUDIO_ENGINE->pos_nfo_at_end_.ninetysixth_notes_
        > AUDIO_ENGINE->pos_nfo_current_.ninetysixth_notes_)
        {
          for (
            int32_t i = AUDIO_ENGINE->pos_nfo_current_.ninetysixth_notes_ + 1;
            i <= AUDIO_ENGINE->pos_nfo_at_end_.ninetysixth_notes_; ++i)
            {
              double ninetysixth_ticks = i * TICKS_PER_NINETYSIXTH_NOTE_DBL;
              double      ratio = (ninetysixth_ticks - AUDIO_ENGINE->pos_nfo_current_.playhead_ticks_) / (AUDIO_ENGINE->pos_nfo_at_end_.playhead_ticks_ - AUDIO_ENGINE->pos_nfo_current_.playhead_ticks_);
              auto midi_time = static_cast<midi_time_t> (
                std::floor (ratio * (double) AUDIO_ENGINE->block_length_));
              if (
                midi_time >= time_nfo.local_offset_
                && midi_time < time_nfo.local_offset_ + time_nfo.nframes_)
                {
                  uint8_t beat_msg = MIDI_CLOCK_BEAT;
                  events.add_raw (&beat_msg, 1, midi_time);
#if 0
                      z_debug (
                        "(i = {}) time {} / {}", i, midi_time,
                        time_nfo.local_offset + time_nfo.nframes_);
#endif
                }
            }
        }

      events.sort ();
    }

  /* append data from each source */
  for (size_t k = 0; k < srcs_.size (); k++)
    {
      const auto * src_port = srcs_[k];
      const auto  &conn = src_connections_[k];
      if (!conn.enabled_)
        continue;

      z_return_if_fail (src_port->id_.type_ == PortType::Event);
      const auto * src_midi_port = dynamic_cast<const MidiPort *> (src_port);

      /* if hardware device connected to track processor input, only allow
       * signal to pass if armed and MIDI channel is valid */
      if (
        src_port->id_.owner_type_ == PortIdentifier::OwnerType::HardwareProcessor
        && owner_type == PortIdentifier::OwnerType::TrackProcessor)
        {
          /* skip if not armed */
          if (
            (recordable_track == nullptr) || !recordable_track->get_recording ())
            continue;

          /* if not set to "all channels", filter-append */
          if (
            (track->is_midi () || track->is_instrument ())
            && !channel_track->channel_->all_midi_channels_)
            {
              events.append_w_filter (
                src_midi_port->midi_events_.active_events_,
                channel_track->channel_->midi_channels_, time_nfo.local_offset_,
                time_nfo.nframes_);
              continue;
            }

          /* otherwise append normally */
        }

      events.append (
        src_midi_port->midi_events_.active_events_, time_nfo.local_offset_,
        time_nfo.nframes_);
    } /* foreach source */

  if (id.is_output ())
    {
      switch (AUDIO_ENGINE->midi_backend_)
        {
#if HAVE_JACK
        case MidiBackend::MIDI_BACKEND_JACK:
          send_midi_events_to_jack (time_nfo.local_offset_, time_nfo.nframes_);
          break;
#endif
        default:
          break;
        }
    }

  /* send UI notification */
  if (events.has_any ())
    {
      if (owner_type == PortIdentifier::OwnerType::TrackProcessor)
        {
          track->trigger_midi_activity_ = true;
        }
    }

  if (time_nfo.local_offset_ + time_nfo.nframes_ == AUDIO_ENGINE->block_length_)
    {
      if (write_ring_buffers_)
        {
          for (auto &ev : events | std::views::reverse)
            {
              if (midi_ring_->write_space () < 1)
                {
                  midi_ring_->skip (1);
                }

              ev.systime_ = g_get_monotonic_time ();
              midi_ring_->write (ev);
            }
        }
      else
        {
          if (events.has_any ())
            {
              last_midi_event_time_ = g_get_monotonic_time ();
              has_midi_events_.store (true);
            }
        }
    }
}