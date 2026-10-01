// SPDX-FileCopyrightText: © 2020-2021, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

#include "dsp/tempo_map.h"
#include "utils/midi.h"
#include "utils/units.h"
#include "utils/utf8_string.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace zrythm::dsp
{

/**
 * @brief Standard MIDI File (SMF) reader/writer.
 *
 * The reading side parses note data into plain structs, with tick values
 * converted to Zrythm's PPQ (960 ticks per quarter note). The writing side
 * serializes the same structs back into an SMF, preceded by tempo and time
 * signature meta events.
 *
 * Only note data is read; tempo, time signature and other meta/controller
 * events are ignored.
 */
class MidiFile
{
public:
  enum class Format
  {
    MIDI0,
    MIDI1,
    MIDI2,
  };

  /** A single note, in Zrythm ticks. */
  struct Note
  {
    midi_byte_t           pitch{};
    midi_byte_t           velocity{};
    std::uint8_t          midi_channel{};
    units::precise_tick_t start_ticks{};
    units::precise_tick_t end_ticks{};
  };

  /** The note content of one SMF track. */
  struct NoteTrack
  {
    utils::Utf8String name;
    std::vector<Note> notes;
  };

  /**
   * @brief Returns whether @p path starts with the Standard MIDI File
   * header magic.
   *
   * This is a cheap check that does not parse the file: files that pass
   * it can still fail to open (see MidiFile()).
   */
  static bool is_midi_file (const std::filesystem::path &path);

  /**
   * @brief Writes the given tracks to a MIDI file at @p path.
   *
   * @param path Destination file.
   * @param format SMF format to write (MIDI 0 or 1).
   * @param bpm Tempo written as a meta event in the first track.
   * @param time_signature Time signature written as a meta event in the
   * first track.
   * @param tracks Note tracks to write.
   * @throw ZrythmException If the arguments are invalid or the file cannot
   * be written.
   */
  static void write_to_file (
    const std::filesystem::path &path,
    Format                       format,
    units::bpm_t                 bpm,
    TimeSignature                time_signature,
    std::span<const NoteTrack>   tracks);

  /**
   * @brief Opens @p path for reading.
   *
   * @throw ZrythmException If the file cannot be read or uses SMPTE timing.
   */
  MidiFile (const std::filesystem::path &path);

  [[nodiscard]] Format format () const { return format_; }

  /**
   * @brief Returns the number of tracks that contain at least one note.
   */
  [[nodiscard]] int num_note_tracks () const;

  /**
   * @brief Parses the note content of a track.
   *
   * @param seq_index Index among the note-bearing tracks (0-based), skipping
   * tracks without notes. Tracks of format 2 files are read the same way
   * as independent format 1 tracks.
   * @return The parsed track. Note-ons without a matching note-off are
   * given a length of one tick.
   * @throw ZrythmException If @p seq_index is out of range.
   */
  [[nodiscard]] NoteTrack read_note_track (int seq_index) const;

private:
  [[nodiscard]] bool track_has_notes (int track_index) const;

  /** Conversion factor from file ticks to Zrythm ticks. */
  [[nodiscard]] double zrythm_ticks_per_file_tick () const
  {
    return static_cast<double> (units::PPQ.in (units::ticks)) / ppqn_;
  }

  juce::MidiFile midi_file_;
  Format         format_ = Format::MIDI0;
  double         ppqn_ = 0.0;
};
}
