// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __IO_MIDI_FILE_H__
#define __IO_MIDI_FILE_H__

#include "zrythm-config.h"

#include <filesystem>

#include "utils/types.h"

#include "juce_wrapper.h"

namespace zrythm::structure
{
namespace arrangement
{
class MidiRegion;
}
namespace tracks
{
class Tracklist;
}
}

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
  into_region (structure::arrangement::MidiRegion &region, int midi_track_idx)
    const;

  /**
   * Exports the Region to a specified MIDI file.
   *
   * FIXME: this needs refactoring. taken out of MidiRegion class.
   *
   * @param full_path Absolute path to the MIDI file.
   * @param export_full Traverse loops and export the MIDI file as it would be
   * played inside Zrythm. If this is false, only the original region (from true
   * start to true end) is exported.
   */
  static void export_midi_region_to_midi_file (
    const structure::arrangement::MidiRegion &region,
    const fs::path                           &full_path,
    int                                       midi_version,
    bool                                      export_full);

  /**
   * Writes the lane to the given MIDI sequence.
   *
   * @param lanes_as_tracks Export lanes as separate MIDI tracks.
   * @param use_track_or_lane_pos Whether to use the track position (or lane
   * position if @ref lanes_as_tracks is true) in the MIDI data. The MIDI track
   * will be set to 1 if false.
   * @param events Track events, if not using lanes as tracks.
   * @param start Events before this position (global ticks) will be skipped.
   * @param end Events after this position (global ticks) will be skipped.
   */
  static void export_midi_lane_to_sequence (
    juce::MidiMessageSequence          &message_sequence,
    const structure::tracks::Tracklist &tracklist,
    const structure::tracks::Track     &track,
    const structure::tracks::TrackLane &lane,
    dsp::MidiEventVector *              events,
    std::optional<double>               start,
    std::optional<double>               end,
    bool                                lanes_as_tracks,
    bool                                use_track_or_lane_pos);

  /**
   * Writes the track to the given MIDI file.
   *
   * @param use_track_pos Whether to use the track position in
   *   the MIDI data. The track will be set to 1 if false.
   * @param events Track events, if not using lanes as tracks or
   *   using track position.
   * @param lanes_as_tracks Export lanes as separate tracks (only possible with
   * MIDI type 1). This will calculate a unique MIDI track number for the
   * region's lane.
   * @param use_track_or_lane_pos Whether to use the track/lane position in the
   * MIDI data. The MIDI track will be set to 1 if false.
   * @param start Events before this position will be skipped.
   * @param end Events after this position will be skipped.
   */
  static void export_track_to_sequence (
    juce::MidiMessageSequence          &message_sequence,
    const structure::tracks::Tracklist &tracklist,
    const structure::tracks::Track     &track,
    dsp::MidiEventVector *              events,
    std::optional<double>               start,
    std::optional<double>               end,
    int                                 track_index,
    bool                                lanes_as_tracks,
    bool                                use_track_pos)
  {
// TODO
#if 0
  std::unique_ptr<dsp::MidiEventVector> own_events;
  if (!lanes_as_tracks && use_track_pos)
    {
      z_return_if_fail (!events);
      midiTrackAddText (mf, track_index, textTrackName, name_.c_str ());
      own_events = std::make_unique<dsp::MidiEventVector> ();
    }

  for (auto &lane : laned_track_mixin_->lanes ())
    {
      lane->write_to_midi_file (
        mf, own_events ? own_events.get () : events, start, end,
        lanes_as_tracks, use_track_pos);
    }

  if (own_events)
    {
      own_events->write_to_midi_file (mf, midi_track_pos);
    }
#endif
  }

private:
  juce::MidiFile midi_file_;
  Format         format_ = Format::MIDI0;
  bool           for_reading_ = false;
};

/**
 * @}
 */

#endif
