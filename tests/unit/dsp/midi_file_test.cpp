// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>

#include "dsp/midi_file.h"
#include "utils/exceptions.h"
#include "utils/io_utils.h"

#include <QTemporaryDir>

#include <gtest/gtest.h>

namespace zrythm::dsp
{
namespace
{

void
set_file_bytes (
  const std::filesystem::path     &path,
  const std::vector<std::uint8_t> &bytes)
{
  utils::io::set_file_contents (
    path, reinterpret_cast<const char *> (bytes.data ()), bytes.size ());
}

void
append_u16_be (std::vector<std::uint8_t> &bytes, const std::uint16_t value)
{
  bytes.push_back (static_cast<std::uint8_t> (value >> 8));
  bytes.push_back (static_cast<std::uint8_t> (value));
}

void
append_u32_be (std::vector<std::uint8_t> &bytes, const std::uint32_t value)
{
  bytes.push_back (static_cast<std::uint8_t> (value >> 24));
  bytes.push_back (static_cast<std::uint8_t> (value >> 16));
  bytes.push_back (static_cast<std::uint8_t> (value >> 8));
  bytes.push_back (static_cast<std::uint8_t> (value));
}

/** Builds a complete SMF from raw track event payloads. */
std::vector<std::uint8_t>
make_smf (
  const std::uint16_t                           format,
  const std::uint16_t                           division,
  const std::vector<std::vector<std::uint8_t>> &track_payloads)
{
  std::vector<std::uint8_t> bytes{ 'M', 'T', 'h', 'd' };
  append_u32_be (bytes, 6);
  append_u16_be (bytes, format);
  append_u16_be (bytes, static_cast<std::uint16_t> (track_payloads.size ()));
  append_u16_be (bytes, division);
  for (const auto &payload : track_payloads)
    {
      bytes.push_back ('M');
      bytes.push_back ('T');
      bytes.push_back ('r');
      bytes.push_back ('k');
      append_u32_be (bytes, static_cast<std::uint32_t> (payload.size ()));
      bytes.insert (bytes.end (), payload.begin (), payload.end ());
    }
  return bytes;
}

/**
 * A track containing a single note, with a track name meta event.
 *
 * @param delta_before_note_on File ticks elapsed before the note starts.
 * @param note_length_in_file_ticks File ticks between note on and note off.
 */
std::vector<std::uint8_t>
make_note_track_payload (
  const std::string_view name,
  const std::uint8_t     pitch,
  const std::uint8_t     velocity,
  const std::uint8_t     delta_before_note_on,
  const std::uint8_t     note_length_in_file_ticks)
{
  std::vector<std::uint8_t> payload;
  payload.push_back (0x00); // delta
  payload.push_back (0xFF);
  payload.push_back (0x03); // track name meta event
  payload.push_back (static_cast<std::uint8_t> (name.size ()));
  payload.insert (payload.end (), name.begin (), name.end ());
  payload.push_back (delta_before_note_on);
  payload.push_back (0x90); // note on, channel 1
  payload.push_back (pitch);
  payload.push_back (velocity);
  payload.push_back (note_length_in_file_ticks); // delta to note off
  payload.push_back (0x80);                      // note off, channel 1
  payload.push_back (pitch);
  payload.push_back (0x40);
  payload.push_back (0x00); // delta
  payload.push_back (0xFF);
  payload.push_back (0x2F); // end of track
  payload.push_back (0x00);
  return payload;
}

/** A track with no note events. */
std::vector<std::uint8_t>
make_empty_track_payload ()
{
  return { 0x00, 0xFF, 0x2F, 0x00 };
}

/** A track with two notes ended by note-ons with velocity 0. */
std::vector<std::uint8_t>
make_vel0_note_off_track_payload ()
{
  return {
    0x00, 0x90, 60,   100,  // note on (pitch 60)
    96,   0x90, 60,   0x00, // note off encoded as velocity 0
    0x00, 0x90, 67,   90,   // note on (pitch 67)
    96,   0x90, 67,   0x00, // note off encoded as velocity 0
    0x00, 0xFF, 0x2F, 0x00  // end of track
  };
}

/** A track with no note-on events at a positive velocity. */
std::vector<std::uint8_t>
make_only_vel0_track_payload ()
{
  return { 0x00, 0x90, 60, 0x00, 0x00, 0xFF, 0x2F, 0x00 };
}

class MidiFileTest : public ::testing::Test
{
protected:
  std::filesystem::path temp_file (const std::string_view name) const
  {
    return temp_dir_path_ / name;
  }

  std::unique_ptr<QTemporaryDir> temp_dir_ =
    utils::io::make_tmp_dir (u8"MidiFileTest");
  std::filesystem::path temp_dir_path_ =
    utils::Utf8String::from_qstring (temp_dir_->path ()).to_path ();
};

TEST_F (MidiFileTest, WriteAndReadBackNotes)
{
  const auto                     path = temp_file ("roundtrip.mid");
  const dsp::MidiFile::NoteTrack lead{
    .name = utils::Utf8String (u8"Lead"),
    .notes = { { .pitch = 60,
                 .velocity = 100,
                 .midi_channel = 0,
                 .start_ticks = units::ticks (0.0),
                 .end_ticks = units::ticks (480.0) },
              { .pitch = 64,
                 .velocity = 90,
                 .midi_channel = 2,
                 .start_ticks = units::ticks (480.0),
                 .end_ticks = units::ticks (960.0) } },
  };
  const dsp::MidiFile::NoteTrack bass{
    .name = utils::Utf8String (u8"Bass"),
    .notes = { { .pitch = 36,
                 .velocity = 80,
                 .midi_channel = 0,
                 .start_ticks = units::ticks (240.0),
                 .end_ticks = units::ticks (720.0) } },
  };
  const std::array<const dsp::MidiFile::NoteTrack, 2> tracks{ lead, bass };

  dsp::MidiFile::write_to_file (
    path, dsp::MidiFile::Format::MIDI1, units::bpm (120.0),
    TimeSignature{ 4, 4 }, tracks);

  EXPECT_TRUE (dsp::MidiFile::is_midi_file (path));

  const dsp::MidiFile midi_file{ path };
  EXPECT_EQ (midi_file.format (), dsp::MidiFile::Format::MIDI1);
  EXPECT_EQ (midi_file.num_note_tracks (), 2);

  const auto parsed_lead = midi_file.read_note_track (0);
  EXPECT_EQ (parsed_lead.name.view (), "Lead");
  ASSERT_EQ (parsed_lead.notes.size (), 2);
  EXPECT_EQ (parsed_lead.notes[0].pitch, 60);
  EXPECT_EQ (parsed_lead.notes[0].velocity, 100);
  EXPECT_EQ (parsed_lead.notes[0].midi_channel, 0);
  EXPECT_DOUBLE_EQ (parsed_lead.notes[0].start_ticks.in (units::ticks), 0.0);
  EXPECT_DOUBLE_EQ (parsed_lead.notes[0].end_ticks.in (units::ticks), 480.0);
  EXPECT_EQ (parsed_lead.notes[1].pitch, 64);
  EXPECT_EQ (parsed_lead.notes[1].velocity, 90);
  EXPECT_EQ (parsed_lead.notes[1].midi_channel, 2);
  EXPECT_DOUBLE_EQ (parsed_lead.notes[1].end_ticks.in (units::ticks), 960.0);

  const auto parsed_bass = midi_file.read_note_track (1);
  EXPECT_EQ (parsed_bass.name.view (), "Bass");
  ASSERT_EQ (parsed_bass.notes.size (), 1);
  EXPECT_EQ (parsed_bass.notes[0].pitch, 36);
  EXPECT_DOUBLE_EQ (parsed_bass.notes[0].start_ticks.in (units::ticks), 240.0);
}

TEST_F (MidiFileTest, ConvertsTicksFromForeignPpqn)
{
  // A note starting at the file's second quarter note (96 file ticks at 96
  // PPQN) and lasting one quarter note
  const auto path = temp_file ("ppqn96.mid");
  set_file_bytes (
    path,
    make_smf (0, 96, { make_note_track_payload ("Test", 60, 100, 96, 96) }));

  const dsp::MidiFile midi_file{ path };
  ASSERT_EQ (midi_file.num_note_tracks (), 1);

  const auto track = midi_file.read_note_track (0);
  ASSERT_EQ (track.notes.size (), 1);
  EXPECT_DOUBLE_EQ (track.notes[0].start_ticks.in (units::ticks), 960.0);
  EXPECT_DOUBLE_EQ (track.notes[0].end_ticks.in (units::ticks), 1920.0);
}

TEST_F (MidiFileTest, SkipsTracksWithoutNotes)
{
  const auto path = temp_file ("with_empty_tracks.mid");
  set_file_bytes (
    path,
    make_smf (
      1, 96,
      { make_empty_track_payload (),
        make_note_track_payload ("Notes", 60, 100, 0, 96),
        make_empty_track_payload () }));

  const dsp::MidiFile midi_file{ path };
  EXPECT_EQ (midi_file.num_note_tracks (), 1);

  const auto track = midi_file.read_note_track (0);
  EXPECT_EQ (track.name.view (), "Notes");
  EXPECT_EQ (track.notes.size (), 1);

  EXPECT_THROW (
    static_cast<void> (midi_file.read_note_track (1)), ZrythmException);
}

TEST_F (MidiFileTest, RejectsSmpteTiming)
{
  const auto path = temp_file ("smpte.mid");
  set_file_bytes (
    path,
    make_smf (0, 0xE728, { make_note_track_payload ("", 60, 100, 0, 96) }));

  EXPECT_THROW (static_cast<void> (dsp::MidiFile{ path }), ZrythmException);
}

TEST_F (MidiFileTest, RejectsNonMidiData)
{
  const auto path = temp_file ("garbage.mid");
  set_file_bytes (path, { 'n', 'o', 't', ' ', 'a', ' ', 'm', 'i', 'd', 'i' });

  EXPECT_FALSE (dsp::MidiFile::is_midi_file (path));
  EXPECT_THROW (static_cast<void> (dsp::MidiFile{ path }), ZrythmException);
}

TEST_F (MidiFileTest, MissingFileIsNotAMidiFile)
{
  const auto path = temp_file ("missing.mid");
  EXPECT_FALSE (dsp::MidiFile::is_midi_file (path));
  EXPECT_THROW (static_cast<void> (dsp::MidiFile{ path }), ZrythmException);
}

TEST_F (MidiFileTest, ZeroLengthNoteGetsOneTickEnd)
{
  auto       payload = make_note_track_payload ("", 60, 100, 0, 0);
  const auto path = temp_file ("zero_length_note.mid");
  set_file_bytes (path, make_smf (0, 96, { payload }));

  const dsp::MidiFile midi_file{ path };
  const auto          track = midi_file.read_note_track (0);
  ASSERT_EQ (track.notes.size (), 1);
  EXPECT_DOUBLE_EQ (track.notes[0].start_ticks.in (units::ticks), 0.0);
  EXPECT_DOUBLE_EQ (track.notes[0].end_ticks.in (units::ticks), 1.0);
}

TEST_F (MidiFileTest, NumNoteTracksForFixtureFiles)
{
  const std::filesystem::path fixtures_dir{ TEST_MIDI_FILES_DIR };

  const MidiFile empty_type_1{ fixtures_dir / "empty_midi_file_type1.mid" };
  EXPECT_EQ (empty_type_1.num_note_tracks (), 0);

  const MidiFile empty_track_plus_data{
    fixtures_dir / "1_empty_track_1_track_with_data.mid"
  };
  EXPECT_EQ (empty_track_plus_data.num_note_tracks (), 1);

  const MidiFile single_track{ fixtures_dir / "1_track_with_data.mid" };
  EXPECT_EQ (single_track.num_note_tracks (), 1);

  const MidiFile two_tracks{
    fixtures_dir / "format_1_two_tracks_with_data.mid"
  };
  EXPECT_EQ (two_tracks.num_note_tracks (), 2);

  const MidiFile real_song{ fixtures_dir / "those_who_remain.mid" };
  EXPECT_EQ (real_song.num_note_tracks (), 1);
}

TEST_F (MidiFileTest, ParsesFixtureFileNotes)
{
  const std::filesystem::path fixtures_dir{ TEST_MIDI_FILES_DIR };

  const MidiFile single_track{ fixtures_dir / "1_track_with_data.mid" };
  const auto     track = single_track.read_note_track (0);
  EXPECT_EQ (track.notes.size (), 3);

  // all notes have a positive length
  for (const auto &note : track.notes)
    {
      EXPECT_GT (
        note.end_ticks.in (units::ticks), note.start_ticks.in (units::ticks));
    }

  const MidiFile real_song{ fixtures_dir / "those_who_remain.mid" };
  const auto     real_track = real_song.read_note_track (0);
  EXPECT_FALSE (real_track.notes.empty ());
  for (const auto &note : real_track.notes)
    {
      EXPECT_LE (note.pitch, 127);
      EXPECT_GE (note.velocity, 1);
      EXPECT_LE (note.velocity, 127);
      EXPECT_LE (note.midi_channel, 15);
    }
}

// Note-ons with velocity 0 are note-offs and must not produce extra notes
TEST_F (MidiFileTest, VelocityZeroNoteOnsAreNoteOffs)
{
  const auto path = temp_file ("vel0_note_offs.mid");
  set_file_bytes (
    path, make_smf (0, 96, { make_vel0_note_off_track_payload () }));

  const dsp::MidiFile midi_file{ path };
  ASSERT_EQ (midi_file.num_note_tracks (), 1);

  const auto track = midi_file.read_note_track (0);
  ASSERT_EQ (track.notes.size (), 2);
  EXPECT_DOUBLE_EQ (track.notes[0].start_ticks.in (units::ticks), 0.0);
  EXPECT_DOUBLE_EQ (track.notes[0].end_ticks.in (units::ticks), 960.0);
  EXPECT_DOUBLE_EQ (track.notes[1].start_ticks.in (units::ticks), 960.0);
  EXPECT_DOUBLE_EQ (track.notes[1].end_ticks.in (units::ticks), 1920.0);
  for (const auto &note : track.notes)
    {
      EXPECT_GE (note.velocity, 1);
    }
}

// A track containing only velocity-0 note-ons carries no note data
TEST_F (MidiFileTest, TrackWithOnlyVelocityZeroNoteOnsHasNoNotes)
{
  const auto path = temp_file ("only_vel0.mid");
  set_file_bytes (path, make_smf (0, 96, { make_only_vel0_track_payload () }));

  const dsp::MidiFile midi_file{ path };
  EXPECT_EQ (midi_file.num_note_tracks (), 0);
}

// Writing rejects time signatures whose denominator is not a valid beat
// unit
TEST_F (MidiFileTest, WriteRejectsInvalidTimeSignature)
{
  const dsp::MidiFile::NoteTrack track{
    .notes = { { .pitch = 60,
                 .velocity = 100,
                 .midi_channel = 0,
                 .start_ticks = units::ticks (0.0),
                 .end_ticks = units::ticks (480.0) } },
  };
  const std::array<const dsp::MidiFile::NoteTrack, 1> tracks{ track };
  const auto path = temp_file ("invalid_ts.mid");

  for (
    const TimeSignature time_signature : {
      TimeSignature{ 4, 0 },
      TimeSignature{ 4, 3 },
      TimeSignature{ 0, 4 }
  })
    {
      EXPECT_THROW (
        static_cast<void> (dsp::MidiFile::write_to_file (
          path, dsp::MidiFile::Format::MIDI1, units::bpm (120.0),
          time_signature, tracks)),
        ZrythmException);
    }
}

// SMF format 0 is a single multi-channel track, so writing multiple tracks
// to it is refused
TEST_F (MidiFileTest, WriteRejectsMidi0WithMultipleTracks)
{
  const dsp::MidiFile::NoteTrack track{
    .notes = { { .pitch = 60,
                 .velocity = 100,
                 .midi_channel = 0,
                 .start_ticks = units::ticks (0.0),
                 .end_ticks = units::ticks (480.0) } },
  };
  const std::array<const dsp::MidiFile::NoteTrack, 2> tracks{ track, track };

  EXPECT_THROW (
    static_cast<void> (dsp::MidiFile::write_to_file (
      temp_file ("midi0_multi.mid"), dsp::MidiFile::Format::MIDI0,
      units::bpm (120.0), TimeSignature{ 4, 4 }, tracks)),
    ZrythmException);
}

// Writing requires at least one track
TEST_F (MidiFileTest, WriteRejectsEmptyTrackList)
{
  const std::array<const dsp::MidiFile::NoteTrack, 0> tracks{};

  EXPECT_THROW (
    static_cast<void> (dsp::MidiFile::write_to_file (
      temp_file ("no_tracks.mid"), dsp::MidiFile::Format::MIDI1,
      units::bpm (120.0), TimeSignature{ 4, 4 }, tracks)),
    ZrythmException);
}

// Writing rejects notes whose fields fall outside the MIDI byte ranges
TEST_F (MidiFileTest, WriteRejectsInvalidNoteFields)
{
  const auto path = temp_file ("invalid_note.mid");
  const auto write_note = [&] (const dsp::MidiFile::Note note) {
    const dsp::MidiFile::NoteTrack track{ .notes = { note } };
    const std::array<const dsp::MidiFile::NoteTrack, 1> tracks{ track };
    dsp::MidiFile::write_to_file (
      path, dsp::MidiFile::Format::MIDI1, units::bpm (120.0),
      TimeSignature{ 4, 4 }, tracks);
  };
  const auto make_note =
    [] (
      const std::uint8_t pitch, const std::uint8_t velocity,
      const std::uint8_t channel) {
      return dsp::MidiFile::Note{
        .pitch = pitch,
        .velocity = velocity,
        .midi_channel = channel,
        .start_ticks = units::ticks (0.0),
        .end_ticks = units::ticks (480.0)
      };
    };

  EXPECT_THROW (
    static_cast<void> (write_note (make_note (128, 100, 0))), ZrythmException);
  EXPECT_THROW (
    static_cast<void> (write_note (make_note (60, 0, 0))), ZrythmException);
  EXPECT_THROW (
    static_cast<void> (write_note (make_note (60, 128, 0))), ZrythmException);
  EXPECT_THROW (
    static_cast<void> (write_note (make_note (60, 100, 16))), ZrythmException);

  const auto zero_length = dsp::MidiFile::Note{
    .pitch = 60,
    .velocity = 100,
    .midi_channel = 0,
    .start_ticks = units::ticks (480.0),
    .end_ticks = units::ticks (480.0)
  };
  EXPECT_THROW (static_cast<void> (write_note (zero_length)), ZrythmException);
}

// A file starting with the SMF header magic counts as a MIDI file even
// when the rest cannot be parsed
TEST_F (MidiFileTest, HeaderMagicAloneIsDetectedAsMidi)
{
  const auto path = temp_file ("magic_only.mid");
  set_file_bytes (path, { 'M', 'T', 'h', 'd', 0, 1, 2, 3 });

  EXPECT_TRUE (dsp::MidiFile::is_midi_file (path));
  EXPECT_THROW (static_cast<void> (dsp::MidiFile{ path }), ZrythmException);
}

} // namespace
} // namespace zrythm::dsp
