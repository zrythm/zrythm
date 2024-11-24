// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __IO_MIDI_FILE_H__
#define __IO_MIDI_FILE_H__

#include "zrythm-config.h"

#include <filesystem>

#include "juce_wrapper.h"
#include "utils/types.h"

class MidiRegion;
class Transport;

/**
 * @addtogroup io
 *
 * @{
 */

/**
 * @brief MIDI file handling.
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

  using TrackIndex = unsigned int;

  /**
   * @brief Construct a new Midi File object for writing.
   */
  MidiFile (Format format);

  /**
   * @brief Construct a new Midi File object for reading.
   *
   * @param path Path to read.
   * @throw ZrythmException If the file could not be read.
   */
  MidiFile (const fs::path &path);

  /**
   * Returns whether the given track in the midi file has data.
   */
  bool track_has_midi_note_events (TrackIndex track_idx) const;

  /**
   * Returns the number of tracks in the MIDI file.
   */
  int get_num_tracks (bool non_empty_only) const;

  [[nodiscard]] Format get_format () const;

  /**
   * @brief Get the PPQN (Parts Per Quarter Note) of the MIDI file.
   *
   * @return int
   * @throw ZrythmException If the MIDI file does not contain a PPQN value.
   */
  int get_ppqn () const;

  /**
   * @brief Reads the contents of the MIDI file into a region.
   *
   * @param region A freshly created region to fill.
   * @param midi_track_idx The index of this track, starting from 0. This will
   * be sequential, ie, if idx 1 is requested and the MIDI file only has tracks
   * 5 and 7, it will use track 7.
   * @throw ZrythmException On error.
   */
  void
  into_region (MidiRegion &region, Transport &transport, int midi_track_idx)
    const;

private:
  juce::MidiFile midi_file_;
  Format         format_ = Format::MIDI0;
  bool           for_reading_ = false;
};

/**
 * @}
 */

#endif
