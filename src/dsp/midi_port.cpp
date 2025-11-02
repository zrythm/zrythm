// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_port.h"
#include "utils/midi.h"

namespace zrythm::dsp
{
MidiPort::MidiPort (utils::Utf8String label, PortFlow flow)
    : Port (std::move (label), PortType::Midi, flow)
{
}

void
init_from (MidiPort &obj, const MidiPort &other, utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Port &> (obj), static_cast<const Port &> (other), clone_type);
}

void
MidiPort::prepare_for_processing (
  sample_rate_t sample_rate,
  nframes_t     max_block_length)
{
  constexpr auto max_midi_events = 24;
  midi_ring_ = std::make_unique<RingBuffer<dsp::MidiEvent>> (max_midi_events);
}

void
MidiPort::release_resources ()
{
  midi_ring_.reset ();
}

void
MidiPort::clear_buffer (std::size_t offset, std::size_t nframes)
{
  midi_events_.active_events_.remove_if (
    [offset, nframes] (const auto &ev) -> bool {
      return ev.time_ >= offset && ev.time_ < (offset + nframes);
    });
}

void
MidiPort::process_block (
  EngineProcessTimeInfo  time_nfo,
  const dsp::ITransport &transport) noexcept
{
  midi_events_.active_events_.clear ();
  midi_events_.dequeue (time_nfo.local_offset_, time_nfo.nframes_);

  auto &events = midi_events_.active_events_;
#if 0
  const auto num_events = midi_events_.active_events_.size ();
  if (num_events > 0)
    {
      z_debug (
        "{} has {} active events after dequeueing", get_node_name (),
        num_events);
    }
#endif

  /* if piano roll keys, add the notes to the piano roll "current notes" (to
   * show pressed keys in the UI) */
// TODO
#if 0
  if (
    owner_type == PortIdentifier::OwnerType::TrackProcessor && is_output ()
    && events.has_any () && CLIP_EDITOR->has_region ()
    && *CLIP_EDITOR->get_track_id () == id_->get_track_id ().value ())
    [[unlikely]]
    {
      bool events_processed = false;
      for (auto &ev : events)
        {
          auto &buf = ev.raw_buffer_;
          if (utils::midi::midi_is_note_on (buf))
            {
              PIANO_ROLL->add_current_note (
                utils::midi::midi_get_note_number (buf));
              events_processed = true;
            }
          else if (utils::midi::midi_is_note_off (buf))
            {
              PIANO_ROLL->remove_current_note (
                utils::midi::midi_get_note_number (buf));
              events_processed = true;
            }
          else if (utils::midi::midi_is_all_notes_off (buf))
            {
              PIANO_ROLL->current_notes_.clear ();
              events_processed = true;
            }
        }
      if (events_processed)
        {
          // EVENTS_PUSH (EventType::ET_PIANO_ROLL_KEY_ON_OFF, nullptr);
        }
    }
#endif

#if 0
  if (is_input () && owner_->should_sum_data_from_backend ())
    {
      // TODO
      // backend_->sum_midi_data (
      // midi_events_,
      // { .start_frame = time_nfo.local_offset_, .nframes = time_nfo.nframes_ },
      // [this] (const auto &ev) {
      // return owner_->are_events_on_midi_channel_approved (
      // utils::midi::midi_get_channel_0_to_15 (ev.raw_buffer_));
      // });
    }
#endif

/* set midi capture if hardware - FIXME move elsewhere */
#if 0
  if (owner_type == PortIdentifier::OwnerType::HardwareProcessor)
    {
      if (events.has_any ())
        {
          /* queue playback if recording and we should record on MIDI input */
          if (
            TRANSPORT->is_recording () && TRANSPORT->isPaused ()
            && TRANSPORT->start_playback_on_midi_input_)
            {
              // EVENTS_PUSH (EventType::ET_TRANSPORT_ROLL_REQUIRED, nullptr);
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
#endif

// TODO: handle MIDI clock (this should be a separate processor)
#if 0
  if (
    ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::MidiClock)
    && is_output ())
    {
      const auto playhead_frames =
        TRANSPORT->playhead_.position_during_processing_rounded ();
      /* continue or start */
      bool start =
        TRANSPORT->isRolling () && !AUDIO_ENGINE->pos_nfo_before_.is_rolling_;
      if (start)
        {
          uint8_t start_msg = utils::midi::MIDI_CLOCK_CONTINUE;
          if (playhead_frames == 0)
            {
              start_msg = utils::midi::MIDI_CLOCK_START;
            }
          events.add_raw (&start_msg, 1, 0);
        }
      else if (
        !TRANSPORT->isRolling () && AUDIO_ENGINE->pos_nfo_before_.is_rolling_)
        {
          uint8_t stop_msg = utils::midi::MIDI_CLOCK_STOP;
          events.add_raw (&stop_msg, 1, 0);
        }

      /* song position */
      dsp::Position playhead_pos{
        static_cast<signed_frame_t> (playhead_frames),
        AUDIO_ENGINE->ticks_per_frame_
      };
      int32_t sixteenth_within_song = playhead_pos.get_total_sixteenths (
        false, AUDIO_ENGINE->frames_per_tick_);
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
              double ninetysixth_ticks =
                i * dsp::Position::TICKS_PER_NINETYSIXTH_NOTE_DBL;
              double      ratio = (ninetysixth_ticks - AUDIO_ENGINE->pos_nfo_current_.playhead_ticks_) / (AUDIO_ENGINE->pos_nfo_at_end_.playhead_ticks_ - AUDIO_ENGINE->pos_nfo_current_.playhead_ticks_);
              auto midi_time = static_cast<midi_time_t> (std::floor (
                ratio * (double) AUDIO_ENGINE->get_block_length ()));
              if (
                midi_time >= time_nfo.local_offset_
                && midi_time < time_nfo.local_offset_ + time_nfo.nframes_)
                {
                  uint8_t beat_msg = utils::midi::MIDI_CLOCK_BEAT;
                  events.add_raw (&beat_msg, 1, midi_time);
#  if 0
                  z_debug ("(i = {}) time {} / {}", i, midi_time, time_nfo.local_offset + time_nfo.nframes_);
#  endif
                }
            }
        }

      events.sort ();
    }
#endif

  /* append data from each source */
  for (const auto &[src_port, conn] : port_sources_)
    {
      if (!conn->enabled_)
        continue;

      events.append (
        src_port->midi_events_.active_events_, time_nfo.local_offset_,
        time_nfo.nframes_);
    } /* foreach source */

  if (num_ring_buffer_readers_ > 0)
    {
      for (auto &ev : events | std::views::reverse)
        {
          if (midi_ring_->write_space () < 1)
            {
              midi_ring_->skip (1);
            }

          // TODO
          // ev.systime_ = Zrythm::getInstance ()->get_monotonic_time_usecs
          // ();
          midi_ring_->write (ev);
        }
    }
}
} // namespace zrythm::dsp
