// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_region.h"
#include "dsp/transport.h"
#include "io/midi_file.h"
#include "utils/exceptions.h"
#include "utils/logger.h"
#include "zrythm.h"

MidiFile::MidiFile (Format format) : format_ (format), for_reading_ (false) { }

MidiFile::MidiFile (const fs::path &path) : for_reading_ (true)
{
  juce::File            file (path.string ());
  juce::FileInputStream in_stream (file);

  int format = 0;
  if (!midi_file_.readFrom (in_stream, true, &format))
    {
      throw ZrythmException (
        fmt::format ("Could not read MIDI file at '{}'", path.string ()));
    }

  format_ = ENUM_INT_TO_VALUE (Format, format);
}

MidiFile::Format
MidiFile::get_format () const
{
  return format_;
}

bool
MidiFile::track_has_midi_note_events (const TrackIndex track_idx) const
{
  z_return_val_if_fail (for_reading_, false);
  const int itrack_idx = static_cast<int> (track_idx);
  z_return_val_if_fail (itrack_idx < midi_file_.getNumTracks (), false);

  const auto * track = midi_file_.getTrack (itrack_idx);

  // Check if the track has any MIDI note on/off events
  return std::any_of (track->begin (), track->end (), [] (const auto &event) {
    return event->message.isNoteOn () || event->message.isNoteOff ();
  });
}

int
MidiFile::get_num_tracks (bool non_empty_only) const
{
  z_return_val_if_fail (for_reading_, 0);

  int num_tracks = midi_file_.getNumTracks ();
  if (non_empty_only)
    {
      int non_empty_tracks = 0;
      for (int i = 0; i < num_tracks; ++i)
        {
          if (track_has_midi_note_events (i))
            {
              ++non_empty_tracks;
            }
        }
      return non_empty_tracks;
    }

  return num_tracks;
}

int
MidiFile::get_ppqn () const
{
  z_return_val_if_fail (for_reading_, 0);

  short time_fmt = midi_file_.getTimeFormat ();

  // If the top bit is clear, this is a PPQN value
  if ((time_fmt & 0x8000) == 0)
    {
      return time_fmt;
    }

  // If the top bit is set, this is SMPTE format, which we don't handle atm
  throw ZrythmException ("SMPTE format not supported yet");
}

void
MidiFile::into_region (
  MidiRegion &region,
  Transport  &transport,
  const int   midi_track_idx) const
{
  const int    num_tracks = midi_file_.getNumTracks ();
  const auto   ppqn = static_cast<double> (get_ppqn ());
  const double transport_ppqn = transport.get_ppqn ();

  int actual_iter = 0;

  for (int i = 0; i < num_tracks; i++)
    {
      if (!track_has_midi_note_events (i))
        {
          continue;
        }

      if (actual_iter != midi_track_idx)
        {
          actual_iter++;
          continue;
        }

      z_debug ("reading MIDI Track {}", i);
      if (i >= 1000)
        {
          throw ZrythmException ("Too many tracks in midi file");
        }

      // set a temp name
      region.set_name (format_str (_ ("Untitled Track {}"), i), false);

      const auto * track = midi_file_.getTrack (i);
      for (const auto * event : *track)
        {
          const auto &msg = event->message;

          /* convert time to zrythm time */
          double ticks = ((double) msg.getTimeStamp () * transport_ppqn) / ppqn;
          Position pos{ ticks };
          Position global_pos = region.pos_;
          global_pos.add_ticks (ticks);
          z_trace (
            "event at {}: {} ", msg.getTimeStamp (), msg.getDescription ());

          int bars = pos.get_bars (true);
          if (ZRYTHM_HAVE_UI && bars > transport.total_bars_ - 8)
            {
              TRANSPORT->update_total_bars (bars + 8, true);
            }

          if (msg.isNoteOff (true))
            {
              auto * mn = region.pop_unended_note (msg.getNoteNumber ());
              if (mn != nullptr)
                {
                  mn->end_pos_setter (&pos);
                  if (global_pos > region.end_pos_)
                    {
                      region.end_pos_setter (&global_pos);
                    }
                }
              else
                {
                  z_info (
                    "Found a Note off event without a corresponding Note on. Skipping...");
                }
            }
          else if (msg.isNoteOn (false))
            {
              region.start_unended_note (
                &pos, nullptr, msg.getNoteNumber (), msg.getVelocity (), false);
            }
          else if (msg.isTrackNameEvent ())
            {
              auto name = msg.getTextFromTextMetaEvent ();
              if (!name.isEmpty ())
                {
                  region.set_name (name.toStdString (), false);
                }
            }
        }

      if (actual_iter == midi_track_idx)
        {
          break;
        }
    }

  if (region.end_pos_ == region.pos_)
    {
      throw ZrythmException ("No events in MIDI region");
    }
  Position loop_end_pos_to_set (region.end_pos_.ticks_ - region.pos_.ticks_);
  region.loop_end_pos_setter (&loop_end_pos_to_set);
}