/*
 * SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2015 Tim Mayberry <mojofunk@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include "common/dsp/chord_descriptor.h"
#include "common/dsp/engine.h"
#include "common/dsp/midi_event.h"
#include "common/dsp/processable_track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/midi.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include <fmt/format.h>

/**
 * Type of MIDI event.
 *
 * These are in order of precedence when sorting.
 *
 * See libs/ardour/midi_buffer.cc for more.
 */
enum class MidiEventType
{
  MIDI_EVENT_TYPE_NOTE_ON,
  MIDI_EVENT_TYPE_NOTE_OFF,
  MIDI_EVENT_TYPE_ALL_NOTES_OFF,
  MIDI_EVENT_TYPE_CONTROLLER,
  MIDI_EVENT_TYPE_PITCHBEND,
  MIDI_EVENT_TYPE_START,
  MIDI_EVENT_TYPE_CONTINUE,
  MIDI_EVENT_TYPE_SONG_POS,
  MIDI_EVENT_TYPE_CLOCK,
  MIDI_EVENT_TYPE_STOP,

  /** Unknown type. */
  MIDI_EVENT_TYPE_RAW,
};

static const char * midi_event_type_strings[] = {
  "pitchbend", "controller", "note off", "note on", "all notes off",
};

void
MidiEventVector::append (
  const MidiEventVector &src,
  const nframes_t        local_offset,
  const nframes_t        nframes)
{
  append_w_filter (src, std::nullopt, local_offset, nframes);
}

void
MidiEventVector::transform_chord_and_append (
  MidiEventVector &src,
  const nframes_t  local_offset,
  const nframes_t  nframes)
{
  {
    const std::lock_guard<crill::spin_mutex> lock (src.lock_);
    for (const auto &src_ev : src.events_)
      {
        /* only copy events inside the current time range */
        if (
          src_ev.time_ < local_offset || src_ev.time_ >= local_offset + nframes)
          {
            if (ZRYTHM_TESTING)
              {
                z_debug (fmt::format (
                  "skipping event: time {} (local offset {} nframes {})",
                  src_ev.time_, local_offset, nframes));
              }
            continue;
          }

        /* make sure there is enough space */
        z_return_if_fail (size () < MAX_MIDI_EVENTS - 6);

        const uint8_t * buf = src_ev.raw_buffer_.data ();

        if (!midi_is_note_on (buf) && !midi_is_note_off (buf))
          continue;

        /* only use middle octave */
        midi_byte_t note_number = midi_get_note_number (buf);
        const auto &descr =
          CHORD_EDITOR->get_chord_from_note_number (note_number);
        if (!descr)
          continue;

        if (midi_is_note_on (buf))
          {
            add_note_ons_from_chord_descr (
              *descr, 1, VELOCITY_DEFAULT, src_ev.time_);
          }
        else if (midi_is_note_off (buf))
          {
            add_note_offs_from_chord_descr (*descr, 1, src_ev.time_);
          }
      }
  }

  /* clear duplicates */
  clear_duplicates ();
}

void
MidiEventVector::append_w_filter (
  const MidiEventVector              &src,
  std::optional<std::array<bool, 16>> channels,
  const nframes_t                     local_offset,
  const nframes_t                     nframes)
{
  {
    const std::lock_guard<crill::spin_mutex> lock (src.lock_);
    for (const auto &src_ev : src.events_)
      {
        /* only copy events inside the current time range */
        if (
          src_ev.time_ < local_offset || src_ev.time_ >= local_offset + nframes)
          {
            if (ZRYTHM_TESTING)
              {
                z_debug (fmt::format (
                  "skipping event: time {} (local offset {} nframes {})",
                  src_ev.time_, local_offset, nframes));
              }
            continue;
          }

        /* if filtering, skip disabled channels */
        if (channels)
          {
            midi_byte_t channel = src_ev.raw_buffer_[0] & 0xf;
            if (!channels->at (channel))
              {
                continue;
              }
          }

        z_return_if_fail (events_.size () < MAX_MIDI_EVENTS);

        push_back (src_ev);
      }
  }

  /* clear duplicates */
  clear_duplicates ();
}

void
MidiEventVector::set_channel (const midi_byte_t channel)
{
  const std::lock_guard<crill::spin_mutex> lock (lock_);
  for (auto &ev : events_)
    {
      /* do this on all MIDI events that have channels */
      auto set_channel = [&] (midi_byte_t byte) {
        if ((ev.raw_buffer_[0] & byte) == byte)
          {
            ev.raw_buffer_[0] = (ev.raw_buffer_[0] & byte) | (channel - 1);
            return true;
          }
        return false;
      };

      /* skip MIDI events starting with the given byte */
      auto skip_channel = [&] (midi_byte_t byte) {
        return (ev.raw_buffer_[0] & byte) == byte;
      };

      if (skip_channel (0xF0))
        continue;
      if (set_channel (0xE0))
        continue;
      if (set_channel (0xD0))
        continue;
      if (set_channel (0xC0))
        continue;
      if (set_channel (0xB0))
        continue;
      if (set_channel (0xA0))
        continue;
      if (set_channel (0x90))
        continue;
      if (set_channel (0x80))
        continue;
    }
}

#if 0
bool
MidiEvents::check_for_note_on (int note, bool queued)
{
  juce::SpinLock::ScopedLockType lock (lock_);

  for (const auto &ev : queued ? queued_events_ : events_)
    {
      midi_byte_t * buf = ev.raw_buffer_;
      if (midi_is_note_on (buf) && midi_get_note_number (buf) == note)
        return true;
    }

  return false;
}

bool
MidiEvents::delete_note_on (int note, bool queued)
{
  juce::SpinLock::ScopedLockType lock (lock_);

  bool  match = false;
  auto &events = queued ? queued_events_ : events_;
  for (auto it = events.begin (); it != events.end ();)
    {
      MidiEvent    &ev = *it;
      midi_byte_t * buf = ev.raw_buffer_;
      if (midi_is_note_on (buf) && midi_get_note_number (buf) == note)
        {
          match = true;
          it = events.erase (it);
        }
      else
        {
          ++it;
        }
    }

  return match;
}

bool
MidiEvents::has_note_on (bool check_main, bool check_queued)
{
  if (check_main)
    {
      for (const auto &ev : events_)
        {
          if (midi_is_note_on (ev.raw_buffer_))
            {
              return true;
            }
        }
    }
  if (check_queued)
    {
      for (const auto &ev : queued_events_)
        {
          if (midi_is_note_on (ev.raw_buffer_))
            {
              return true;
            }
        }
    }

  return 0;
}
#endif

void
MidiEvents::dequeue (const nframes_t local_offset, const nframes_t nframes)
{
  if (ZRYTHM_TESTING) [[unlikely]]
    {
      assert (active_events_.capacity () >= queued_events_.size ());
    }

  active_events_.append (queued_events_, local_offset, nframes);
  queued_events_.remove_if ([local_offset, nframes] (const MidiEvent &ev) {
    return ev.time_ >= local_offset && ev.time_ < local_offset + nframes;
  });
}

void
MidiEventVector::
  add_all_notes_off (midi_byte_t channel, midi_time_t time, bool with_lock)
{
  z_return_if_fail (channel > 0 && channel <= 16);

  MidiEvent ev (
    (midi_byte_t) (MIDI_CH1_CTRL_CHANGE | (channel - 1)), MIDI_ALL_NOTES_OFF,
    0x00, time);
  z_return_if_fail (midi_is_all_notes_off (ev.raw_buffer_.data ()));

  if (with_lock)
    {
      lock_.lock ();
    }

  events_.push_back (ev);

  if (with_lock)
    {
      lock_.unlock ();
    }
}

void
MidiEventVector::panic ()
{
  // if (zrythm_app)
  //   {
  //     z_return_if_fail (current_thread_id.get () ==
  //     zrythm_app->gtk_thread_id_);
  //   }

  while (lock_.try_lock ())
    {
      g_usleep (10);
    }
  panic_without_lock ();
  lock_.unlock ();
}

void
MidiEventVector::write_to_midi_file (MIDI_FILE * mf, int midi_track) const
{
  const std::lock_guard<crill::spin_mutex> lock (lock_);
  z_return_if_fail (midi_track > 0);

  int last_time = 0;
  for (const MidiEvent &ev : events_)
    {
      BYTE buf[] = { ev.raw_buffer_[0], ev.raw_buffer_[1], ev.raw_buffer_[2] };
      int  delta_time = (int) ev.time_ - last_time;
      midiTrackAddRaw (
        mf, midi_track, 3, buf,
        /* move ptr */
        true, delta_time);
      last_time += delta_time;
    }
}

#if HAVE_JACK
void
MidiEventVector::copy_to_jack (
  const nframes_t local_start_frames,
  const nframes_t nframes,
  void *          buff)
{
  const std::lock_guard<crill::spin_mutex> lock (lock_);
  /*jack_midi_clear_buffer (buff);*/

  for (const auto &ev : events_)
    {
      jack_midi_data_t midi_data[3];

      if (
        ev.time_ < local_start_frames
        || ev.time_ >= local_start_frames + nframes)
        {
          continue;
        }

      std::copy_n (midi_data, ev.raw_buffer_sz_, (jack_midi_data_t *) buff);
      jack_midi_event_write (buff, ev.time_, midi_data, ev.raw_buffer_sz_);
#  if 0
      z_info (
        "wrote MIDI event to JACK MIDI out at %d", ev.time);
#  endif
    }
}
#endif

void
MidiEventVector::
  add_note_off (midi_byte_t channel, midi_byte_t note_pitch, midi_time_t time)
{
  z_return_if_fail (channel > 0);
  MidiEvent ev (
    (midi_byte_t) (MIDI_CH1_NOTE_OFF | (channel - 1)), note_pitch, 90, time);
  z_return_if_fail (midi_is_note_off (ev.raw_buffer_.data ()));
  push_back (ev);
}

void
MidiEventVector::add_song_pos (int64_t total_sixteenths, midi_time_t time)
{
  add_simple (
    MIDI_SONG_POSITION, total_sixteenths & 0x7f /* LSB */,
    (total_sixteenths >> 7) & 0x7f /* MSB */, time);
}

void
MidiEventVector::add_raw (uint8_t * buf, size_t buf_sz, midi_time_t time)
{
  if (buf_sz > 3)
    {
      midi_print (buf, buf_sz);
      z_return_if_reached ();
    }

  MidiEvent ev;
  ev.time_ = time;
  for (size_t i = 0; i < buf_sz; i++)
    {
      ev.raw_buffer_[i] = buf[i];
    }
  ev.raw_buffer_sz_ = buf_sz;
  push_back (ev);
}

void
MidiEventVector::add_control_change (
  uint8_t     channel,
  uint8_t     controller,
  uint8_t     control,
  midi_time_t time)
{
  add_simple (
    (midi_byte_t) (MIDI_CH1_CTRL_CHANGE | (channel - 1)), controller, control,
    time);
}

void
MidiEventVector::
  add_pitchbend (midi_byte_t channel, uint32_t pitchbend, midi_time_t time)
{
  z_return_if_fail (pitchbend < 0x4000 && channel > 0);

  MidiEvent ev;
  ev.time_ = time;
  ev.raw_buffer_[0] = (midi_byte_t) (MIDI_CH1_PITCH_WHEEL_RANGE | (channel - 1));
  midi_get_bytes_from_combined (
    pitchbend, &ev.raw_buffer_[1], &ev.raw_buffer_[2]);
  ev.raw_buffer_sz_ = 3;

  push_back (ev);
}

void
MidiEventVector::add_channel_pressure (
  midi_byte_t channel,
  midi_byte_t value,
  midi_time_t time)
{
  z_return_if_fail (channel > 0);

  MidiEvent ev;
  ev.time_ = time;
  ev.raw_buffer_[0] = (midi_byte_t) (MIDI_CH1_CHAN_AFTERTOUCH | (channel - 1));
  ev.raw_buffer_[1] = value;
  ev.raw_buffer_sz_ = 2;

  push_back (ev);
}

static inline MidiEventType
get_event_type (const std::array<midi_byte_t, 3> _short_msg)
{
  const auto short_msg = _short_msg.data ();
  if (midi_is_note_off (short_msg))
    return MidiEventType::MIDI_EVENT_TYPE_NOTE_OFF;
  else if (midi_is_note_on (short_msg))
    return MidiEventType::MIDI_EVENT_TYPE_NOTE_ON;
  /* note: this is also a controller */
  else if (midi_is_all_notes_off (short_msg))
    return MidiEventType::MIDI_EVENT_TYPE_ALL_NOTES_OFF;
  /* note: this is also a controller */
  else if (midi_is_pitch_wheel (short_msg))
    return MidiEventType::MIDI_EVENT_TYPE_PITCHBEND;
  else if (midi_is_controller (short_msg))
    return MidiEventType::MIDI_EVENT_TYPE_CONTROLLER;
  else if (midi_is_song_position_pointer (short_msg))
    return MidiEventType::MIDI_EVENT_TYPE_SONG_POS;
  else if (midi_is_start (short_msg))
    return MidiEventType::MIDI_EVENT_TYPE_START;
  else if (midi_is_stop (short_msg))
    return MidiEventType::MIDI_EVENT_TYPE_STOP;
  else if (midi_is_continue (short_msg))
    return MidiEventType::MIDI_EVENT_TYPE_CONTINUE;
  else if (midi_is_clock (short_msg))
    return MidiEventType::MIDI_EVENT_TYPE_CLOCK;
  else
    return MidiEventType::MIDI_EVENT_TYPE_RAW;
}

void
MidiEventVector::sort ()
{
  const std::lock_guard<crill::spin_mutex> lock (lock_);

  std::sort (
    events_.begin (), events_.end (),
    [] (const MidiEvent &a, const MidiEvent &b) {
      if (a.time_ == b.time_) [[unlikely]]
        {
          MidiEventType a_type = get_event_type (a.raw_buffer_);
          MidiEventType b_type = get_event_type (b.raw_buffer_);
          (void) midi_event_type_strings;
#if 0
      z_debug ("a type {}, b type {}",
        midi_event_type_strings[a_type],
        midi_event_type_strings[b_type]);
#endif
          return (int) a_type < (int) b_type;
        }
      return a.time_ < b.time_;
    });
}

void
MidiEventVector::add_note_on (
  uint8_t     channel,
  uint8_t     note_pitch,
  uint8_t     velocity,
  midi_time_t time)
{
  z_return_if_fail (channel > 0);
#if 0
  z_info (fmt::format (
    "ch {}, pitch {}, vel {}, time {}", channel, note_pitch, velocity, time));
#endif

  MidiEvent ev (
    (midi_byte_t) (MIDI_CH1_NOTE_ON | (channel - 1)), note_pitch, velocity,
    time);
  z_return_if_fail (midi_is_note_on (ev.raw_buffer_.data ()));

  push_back (ev);
}

void
MidiEventVector::add_note_ons_from_chord_descr (
  const ChordDescriptor &descr,
  midi_byte_t            channel,
  midi_byte_t            velocity,
  midi_time_t            time)
{
#if 0
  z_info (
    "%s: vel %"PRIu8", time %"PRIu32,
    __func__, velocity, _time);
#endif

  for (int i = 0; i < CHORD_DESCRIPTOR_MAX_NOTES; i++)
    {
      if (descr.notes_[i])
        {
          add_note_on (channel, i + 36, velocity, time);
        }
    }
}

void
MidiEventVector::add_note_offs_from_chord_descr (
  const ChordDescriptor &descr,
  midi_byte_t            channel,
  midi_time_t            time)
{
  for (int i = 0; i < CHORD_DESCRIPTOR_MAX_NOTES; i++)
    {
      if (descr.notes_[i])
        {
          add_note_off (channel, i + 36, time);
        }
    }
}

ATTR_REALTIME
void
MidiEventVector::
  add_event_from_buf (midi_time_t time, midi_byte_t * buf, int buf_size)
{
  if (buf_size != 3) [[unlikely]]
    {
#if 0
      z_debug (
        "buf size of %d received (%"PRIu8" %"
        PRIu8" %"PRIu8"), expected 3, skipping...",
        buf_size > 0 ? buf[0] : 0,
        buf_size > 1 ? buf[1] : 0,
        buf_size > 2 ? buf[2] : 0, buf_size);
#endif

      return;
    }

  midi_byte_t type = buf[0] & 0xf0;
  auto        channel = (midi_byte_t) ((buf[0] & 0xf) + 1);
  switch (type)
    {
    case MIDI_CH1_NOTE_ON:
      /* velocity == 0 means note off */
      if (buf[2] == 0)
        goto note_off;

      add_note_on (channel, buf[1], buf[2], time);
      break;
    case MIDI_CH1_NOTE_OFF:
note_off:
      add_note_off (channel, buf[1], time);
      break;
    case MIDI_CH1_PITCH_WHEEL_RANGE:
      add_pitchbend (channel, midi_get_14_bit_value (buf), time);
      break;
    case MIDI_CH1_CHAN_AFTERTOUCH:
      add_channel_pressure (channel, buf[1], time);
      break;
    case MIDI_SYSTEM_MESSAGE:
      /* ignore active sensing */
      if (buf[0] != 0xFE)
        {
#if 0
          print_unknown_event_message (buf, buf_size);
#endif
        }
      break;
    case MIDI_CH1_CTRL_CHANGE:
      add_control_change (1, buf[1], buf[2], time);
      break;
    case MIDI_CH1_POLY_AFTERTOUCH:
      /* TODO unimplemented */
      /* fallthrough */
    default:
      add_raw (buf, (size_t) buf_size, time);
      break;
    }
}

void
MidiEvent::set_velocity (midi_byte_t vel)
{
  raw_buffer_[2] = vel;
}

void
MidiEvent::print () const
{
  char msg[400];
  midi_print_to_str (raw_buffer_.data (), raw_buffer_sz_, msg);
  z_info ("{} | time: {}", msg, time_);
}

void
MidiEventVector::print () const
{
  const std::lock_guard<crill::spin_mutex> lock (lock_);
  for (const auto &ev : events_)
    {
      ev.print ();
    }
}

void
MidiEvents::panic_all ()
{
  z_info ("~ midi panic all ~");

  AUDIO_ENGINE->midi_editor_manual_press_->midi_events_.queued_events_.panic ();

  for (auto track : TRACKLIST->tracks_ | type_is<ProcessableTrack> ())
    {
      if (track->has_piano_roll () || track->is_chord ())
        {
          track->processor_->piano_roll_->midi_events_.queued_events_.panic ();
        }
    }
}

void
MidiEventVector::clear_duplicates ()
{
  const std::lock_guard<crill::spin_mutex> lock (lock_);

  /* push duplicates to the end of the vector and get iterator to first
   * duplicate*/
  auto last = std::unique (events_.begin (), events_.end ());

  /* remove duplicates */
  events_.erase (last, events_.end ());
}