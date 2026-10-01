// SPDX-FileCopyrightText: © 2020-2021, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

#include <fmt/std.h>

#include "dsp/midi_file.h"
#include "utils/exceptions.h"
#include "utils/logger.h"
#include "utils/views.h"

namespace zrythm::dsp
{

bool
MidiFile::is_midi_file (const std::filesystem::path &path)
{
  const auto file = utils::Utf8String::from_path (path).to_juce_file ();
  if (!file.existsAsFile ())
    {
      return false;
    }

  juce::FileInputStream stream (file);
  if (!stream.openedOk ())
    {
      return false;
    }

  std::array<char, 4> magic{};
  if (
    stream.read (magic.data (), static_cast<int> (magic.size ()))
    != static_cast<int> (magic.size ()))
    {
      return false;
    }
  static constexpr std::string_view expected{ "MThd" };
  return std::string_view{ magic.data (), magic.size () } == expected;
}

void
MidiFile::write_to_file (
  const std::filesystem::path &path,
  Format                       format,
  units::bpm_t                 bpm,
  TimeSignature                time_signature,
  std::span<const NoteTrack>   tracks)
{
  if (!(bpm.in (units::bpm) > 0.0))
    {
      throw utils::ZrythmException (
        fmt::format (
          "Invalid tempo {} for writing a MIDI file", bpm.in (units::bpm)));
    }
  if (format == Format::MIDI2)
    {
      throw utils::ZrythmException ("MIDI 2 files cannot be written");
    }
  if (tracks.empty ())
    {
      throw utils::ZrythmException (
        "At least one note track is required for writing a MIDI file");
    }
  if (format == Format::MIDI0 && tracks.size () > 1)
    {
      throw utils::ZrythmException (
        "MIDI format 0 files cannot contain multiple tracks");
    }
  try
    {
      throw_if_invalid_time_signature (time_signature);
    }
  catch (const std::invalid_argument &e)
    {
      throw utils::ZrythmException (
        fmt::format (
          "Invalid time signature for writing a MIDI file: {}", e.what ()));
    }
  for (const auto [track_index, track] : utils::views::enumerate (tracks))
    {
      for (const auto [note_index, note] : utils::views::enumerate (track.notes))
        {
          if (
            note.pitch > 127 || note.velocity < 1 || note.velocity > 127
            || note.midi_channel > 15)
            {
              throw utils::ZrythmException (
                fmt::format (
                  "Note {} of note track {} cannot be written to a MIDI file: pitch, velocity or channel out of range",
                  note_index, track_index));
            }
          if (note.end_ticks <= note.start_ticks)
            {
              throw utils::ZrythmException (
                fmt::format (
                  "Note {} of note track {} cannot be written to a MIDI file: the note length must be positive",
                  note_index, track_index));
            }
        }
    }

  juce::MidiFile midi_file;
  midi_file.setTicksPerQuarterNote (
    static_cast<int> (units::PPQ.in (units::ticks)));

  bool first_track = true;
  for (const auto &track : tracks)
    {
      juce::MidiMessageSequence sequence;

      if (first_track)
        {
          sequence.addEvent (
            juce::MidiMessage::timeSignatureMetaEvent (
              time_signature.numerator, time_signature.denominator));
          sequence.addEvent (
            juce::MidiMessage::tempoMetaEvent (
              static_cast<int> (std::round (60'000'000.0 / bpm.in (units::bpm)))));
          first_track = false;
        }

      if (!track.name.empty ())
        {
          sequence.addEvent (
            juce::MidiMessage::textMetaEvent (3, track.name.to_juce_string ()));
        }
      for (const auto &note : track.notes)
        {
          const auto midi_channel = static_cast<int> (note.midi_channel) + 1;
          sequence.addEvent (
            juce::MidiMessage::noteOn (
              midi_channel, note.pitch, static_cast<juce::uint8> (note.velocity)),
            note.start_ticks.in (units::ticks));
          sequence.addEvent (
            juce::MidiMessage::noteOff (
              midi_channel, note.pitch, static_cast<juce::uint8> (0)),
            note.end_ticks.in (units::ticks));
        }

      midi_file.addTrack (sequence);
    }

  const auto file = utils::Utf8String::from_path (path).to_juce_file ();
  auto       output_stream = file.createOutputStream ();
  if (output_stream == nullptr)
    {
      throw utils::ZrythmException (
        fmt::format ("Could not create output stream for '{}'", path));
    }
  if (!midi_file.writeTo (*output_stream, format == Format::MIDI0 ? 0 : 1))
    {
      throw utils::ZrythmException (
        fmt::format ("Could not write MIDI file at '{}'", path));
    }
}

MidiFile::MidiFile (const std::filesystem::path &path)
{
  const auto file = utils::Utf8String::from_path (path).to_juce_file ();
  juce::FileInputStream in_stream (file);
  if (!in_stream.openedOk ())
    {
      throw utils::ZrythmException (
        fmt::format ("Could not open MIDI file at '{}'", path));
    }

  int format = 0;
  if (!midi_file_.readFrom (in_stream, true, &format))
    {
      throw utils::ZrythmException (
        fmt::format ("Could not read MIDI file at '{}'", path));
    }
  if (format < 0 || format > 2)
    {
      throw utils::ZrythmException (
        fmt::format (
          "MIDI file at '{}' uses unsupported format {}", path, format));
    }
  format_ = static_cast<Format> (format);

  const short time_format = midi_file_.getTimeFormat ();
  if ((time_format & 0x8000) != 0)
    {
      throw utils::ZrythmException (
        fmt::format (
          "MIDI file at '{}' uses SMPTE timing, which is not supported", path));
    }
  ppqn_ = static_cast<double> (time_format);
  if (ppqn_ <= 0.0)
    {
      throw utils::ZrythmException (
        fmt::format ("MIDI file at '{}' has an invalid time division", path));
    }
}

bool
MidiFile::track_has_notes (const int track_index) const
{
  const auto * track = midi_file_.getTrack (track_index);
  return std::ranges::any_of (*track, [] (const auto &event) {
    return event->message.isNoteOn ();
  });
}

int
MidiFile::num_note_tracks () const
{
  const int num_tracks = midi_file_.getNumTracks ();
  int       note_tracks = 0;
  for (const auto i : std::views::iota (0, num_tracks))
    {
      if (track_has_notes (i))
        {
          ++note_tracks;
        }
    }
  return note_tracks;
}

MidiFile::NoteTrack
MidiFile::read_note_track (const int seq_index) const
{
  const auto scale = zrythm_ticks_per_file_tick ();

  int note_track_count = 0;
  for (const auto i : std::views::iota (0, midi_file_.getNumTracks ()))
    {
      if (!track_has_notes (i))
        {
          continue;
        }

      if (note_track_count != seq_index)
        {
          ++note_track_count;
          continue;
        }

      NoteTrack    track_data;
      const auto * sequence = midi_file_.getTrack (i);
      for (
        const auto event_index : std::views::iota (0, sequence->getNumEvents ()))
        {
          const auto &msg = sequence->getEventPointer (event_index)->message;

          if (msg.isNoteOn ())
            {
              const auto start_ticks = msg.getTimeStamp () * scale;
              auto       end_ticks =
                sequence->getTimeOfMatchingKeyUp (event_index) * scale;
              if (end_ticks <= start_ticks)
                {
                  end_ticks = start_ticks + 1.0;
                }

              track_data.notes.push_back (
                { .pitch = static_cast<midi_byte_t> (msg.getNoteNumber ()),
                  .velocity = msg.getVelocity (),
                  .midi_channel =
                    static_cast<std::uint8_t> (msg.getChannel () - 1),
                  .start_ticks = units::ticks (start_ticks),
                  .end_ticks = units::ticks (end_ticks) });
            }
          else if (msg.isTrackNameEvent ())
            {
              track_data.name = utils::Utf8String::from_juce_string (
                msg.getTextFromTextMetaEvent ());
            }
        }

      std::ranges::sort (track_data.notes, {}, &Note::start_ticks);
      return track_data;
    }

  throw utils::ZrythmException (
    fmt::format (
      "MIDI file has no note track at index {} ({} note tracks found)",
      seq_index, num_note_tracks ()));
}

}
