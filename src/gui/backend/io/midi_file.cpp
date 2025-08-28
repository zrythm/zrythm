// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "gui/backend/io/midi_file.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/midi_region.h"
#include "utils/exceptions.h"
#include "utils/logger.h"

MidiFile::MidiFile (Format format) : format_ (format), for_reading_ (false) { }

MidiFile::MidiFile (const fs::path &path) : for_reading_ (true)
{
  juce::File file = utils::Utf8String::from_path (path).to_juce_file ();
  juce::FileInputStream in_stream (file);

  int format = 0;
  if (!midi_file_.readFrom (in_stream, true, &format))
    {
      throw ZrythmException (
        fmt::format ("Could not read MIDI file at '{}'", path));
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
  return std::ranges::any_of (
    track->begin (), track->end (), [] (const auto &event) {
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
  structure::arrangement::MidiRegion &region,
  const int                           midi_track_idx) const
{
// TODO
#if 0
  const int   num_tracks = midi_file_.getNumTracks ();
  const auto  ppqn = static_cast<double> (get_ppqn ());
  const auto &tempo_map = region.get_tempo_map ();

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
      region.regionMixin ()->name ()->setName (
        format_qstr (QObject::tr ("Untitled Track {}"), i));

      const auto * track = midi_file_.getTrack (i);
      track->getTimeOfMatchingKeyUp (0);
      for (const auto &[event_index, event] : utils::views::enumerate (*track))
        {
          const auto &msg = event->message;

          const auto get_msg_time_in_ticks = [ppqn] (const double msg_timestamp) {
            /* convert time to zrythm time */
            return (msg_timestamp * dsp::TempoMap::get_ppq ()) / ppqn;
          };

          double ticks = get_msg_time_in_ticks (msg.getTimeStamp ());
          auto   global_pos = region.position ()->ticks ();
          global_pos += ticks;
          z_trace ("event at {}: {} ", ticks, msg.getDescription ());

          if (msg.isNoteOff (true))
            {
              // handled below via getTimeOfMatchingKeyUp()
            }
          else if (msg.isNoteOn (false))
            {
              const auto note_on_time = ticks;
              const auto note_off_time = get_msg_time_in_ticks (
                track->getTimeOfMatchingKeyUp (static_cast<int> (event_index)));
              auto mn_builder = std::move (
                PROJECT->getArrangerObjectFactory ()
                  ->get_builder<structure::arrangement::MidiNote> ());
              auto mn =
                mn_builder.with_start_ticks (note_on_time)
                  .with_end_ticks (note_off_time)
                  .with_pitch (msg.getNoteNumber ())
                  .with_velocity (msg.getVelocity ())
                  .build_in_registry ();
              region.add_object (mn);
            }
          else if (msg.isTrackNameEvent ())
            {
              auto name = msg.getTextFromTextMetaEvent ();
              if (!name.isEmpty ())
                {
                  region.regionMixin ()->name ()->setName (
                    utils::Utf8String::from_juce_string (name).to_qstring ());
                }
            }
        }

      if (actual_iter == midi_track_idx)
        {
          break;
        }
    }

// rest TODO
#  if 0
  // set end position to last event
  region.regionMixin ()->bounds ()->length ()->setTicks (
    structure::arrangement::get_last_midi_note (region.get_children_view ())
      ->get_end_position_ticks ());

  if (region.get_children_view ().empty ())
    {
      throw ZrythmException ("No events in MIDI region");
    }
  Position loop_end_pos_to_set (
    region.end_pos_->ticks_ - region.get_position ().ticks_,
    AUDIO_ENGINE->frames_per_tick_);
  region.loop_end_position_setter_validated (
    loop_end_pos_to_set, AUDIO_ENGINE->ticks_per_frame_);
#  endif
#endif
}

void
MidiFile::export_midi_region_to_midi_file (
  const structure::arrangement::MidiRegion &region,
  const fs::path                           &full_path,
  int                                       midi_version,
  const bool                                export_full)
{
  juce::MidiMessageSequence sequence;

  const auto &tempo_map = region.get_tempo_map ();

  // Write tempo/time signature information
  // FIXME: doesn't take into account tempo/time signature changes
  sequence.addEvent (
    juce::MidiMessage::timeSignatureMetaEvent (
      tempo_map.time_signature_at_tick (0).numerator,
      tempo_map.time_signature_at_tick (0).denominator));
  sequence.addEvent (
    juce::MidiMessage::tempoMetaEvent (
      60'000'000 / static_cast<int> (tempo_map.tempo_at_tick (0))));

  dsp::MidiEventVector events;
  region.add_midi_region_events (
    events, std::nullopt, std::nullopt, false, export_full);
  events.write_to_midi_sequence (sequence, true);

  juce::MidiFile mf;
  mf.setTicksPerQuarterNote (dsp::TempoMap::get_ppq ());
  mf.addTrack (sequence);
  const auto juce_file =
    utils::Utf8String::from_path (full_path).to_juce_file ();
  auto output_stream = juce_file.createOutputStream ();
  if (!output_stream)
    {
      throw std::runtime_error ("Failed to create output stream");
    }
  mf.writeTo (*output_stream, midi_version);
}

void
MidiFile::export_midi_lane_to_sequence (
  juce::MidiMessageSequence          &message_sequence,
  const structure::tracks::Tracklist &tracklist,
  const structure::tracks::Track     &track,
  const structure::tracks::TrackLane &lane,
  dsp::MidiEventVector *              events,
  std::optional<double>               start,
  std::optional<double>               end,
  bool                                lanes_as_tracks,
  bool                                use_track_or_lane_pos)
{
// TODO
#if 0
  auto calculate_lane_idx_for_midi_serialization =
    [&tracklist, &track, &lane] () {
      const auto lane_index = track.lanes ().get_lane_index (lane.get_uuid ());
      int        pos = 1;
      for (const auto &cur_track_var : tracklist.get_track_span ())
        {
          auto * cur_track = structure::tracks::from_variant (cur_track_var);
          if (cur_track->get_uuid () == track.get_uuid ())
            {
              pos += lane_index;
              break;
            }

          pos += cur_track->lanes () ? cur_track->lanes ()->rowCount () : 0;
        }

      return pos;
    };

  auto midi_track_pos = tracklist.get_track_index (track.get_uuid ());
  std::unique_ptr<dsp::MidiEventVector> own_events;
  if (lanes_as_tracks)
    {
      z_return_if_fail (!events);
      midi_track_pos = calculate_lane_idx_for_midi_serialization ();
      own_events = std::make_unique<dsp::MidiEventVector> ();
    }
  else if (!use_track_or_lane_pos)
    {
      z_return_if_fail (events);
      midi_track_pos = 1;
    }
  /* else if using track positions */
  else
    {
      z_return_if_fail (events);
    }

  /* All data is written out to tracks not channels. We therefore set the
   * current channel before writing data out. Channel assignments can change any
   * number of times during the file, and affect all tracks messages until it is
   * changed. */
  // midiFileSetTracksDefaultChannel (mf, midi_track_pos, MIDI_CHANNEL_1);

  /* add track name */
  if (lanes_as_tracks && use_track_or_lane_pos)
    {
      // const auto midi_track_name =
      //   fmt::format ("{} - {}", track.get_name (), lane.name());
      // midiTrackAddText (
      // mf, midi_track_pos, textTrackName, midi_track_name.c_str ());
    }

  for (
    auto * region :
    lane.structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiRegion>::get_children_view ())
    {
      /* skip regions not inside the given range */
      if (start)
        {
          if (
            (region->position ()->ticks ()
             + region->regionMixin ()->bounds ()->length ()->ticks ())
            < *start)
            continue;
        }
      if (end)
        {
          if (region->position ()->ticks () > *end)
            continue;
        }

      region->add_midi_region_events (
        own_events ? *own_events : *events, start, end, true, true);
    }

  if (own_events)
    {
      // TODO
      // own_events->write_to_midi_file (mf, midi_track_pos);
    }
#endif
}
