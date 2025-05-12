// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "dsp/position.h"
#include "gui/dsp/laned_track.h"
#include "gui/dsp/midi_region.h"
#include "gui/dsp/recordable_track.h"
#include "utils/types.h"

class MidiEvents;
class Velocity;
using MIDI_FILE = void;

#define DEFINE_PIANO_ROLL_TRACK_QML_PROPERTIES(ClassType) \
  DEFINE_RECORDABLE_TRACK_QML_PROPERTIES (ClassType)

/**
 * Interface for a piano roll track.
 */
class PianoRollTrack
    : virtual public RecordableTrack,
      public LanedTrackImpl<MidiLane>
{
  Z_DISABLE_COPY_MOVE (PianoRollTrack)
protected:
  // = default deletes it for some reason on gcc
  PianoRollTrack () noexcept { }

public:
  ~PianoRollTrack () override = default;

  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  /**
   * Writes the track to the given MIDI file.
   *
   * @param use_track_pos Whether to use the track position in
   *   the MIDI data. The track will be set to 1 if false.
   * @param events Track events, if not using lanes as tracks or
   *   using track position.
   * @param start Events before this position will be skipped.
   * @param end Events after this position will be skipped.
   */
  void write_to_midi_file (
    MIDI_FILE *       mf,
    MidiEventVector * events,
    const Position *  start,
    const Position *  end,
    bool              lanes_as_tracks,
    bool              use_track_pos);

  /**
   * Fills in the array with all the velocities in the project that are within
   * or outside the range given.
   *
   * @param inside Whether to find velocities inside the range or outside.
   */
  void get_velocities_in_range (
    const Position *         start_pos,
    const Position *         end_pos,
    std::vector<Velocity *> &velocities,
    bool                     inside) const;

  void clear_objects () override;

  void get_regions_in_range (
    std::vector<Region *> &regions,
    const Position *       p1,
    const Position *       p2) override;

protected:
  void
  copy_members_from (const PianoRollTrack &other, ObjectCloneType clone_type);

  void set_playback_caches () override;

private:
  static constexpr auto kDrumModeKey = "drumMode"sv;
  static constexpr auto kMidiChKey = "midiCh"sv;
  static constexpr auto kPassthroughMidiInputKey = "passthroughMidiInput"sv;
  friend void           to_json (nlohmann::json &j, const PianoRollTrack &track)
  {
    j[kDrumModeKey] = track.drum_mode_;
    j[kMidiChKey] = track.midi_ch_;
    j[kPassthroughMidiInputKey] = track.passthrough_midi_input_;
  }
  friend void from_json (const nlohmann::json &j, PianoRollTrack &track)
  {
    j.at (kDrumModeKey).get_to (track.drum_mode_);
    j.at (kMidiChKey).get_to (track.midi_ch_);
    j.at (kPassthroughMidiInputKey).get_to (track.passthrough_midi_input_);
  }

public:
  /**
   * Whether drum mode in the piano roll is enabled for this track.
   */
  bool drum_mode_ = false;

  /** MIDI channel (1-16) */
  midi_byte_t midi_ch_ = 1;

  /**
   * If true, the input received will not be changed to the selected MIDI
   * channel.
   *
   * If false, all input received will have its channel changed to the
   * selected MIDI channel.
   */
  bool passthrough_midi_input_ = false;
};

using PianoRollTrackVariant = std::variant<MidiTrack, InstrumentTrack>;
using PianoRollTrackPtrVariant = to_pointer_variant<PianoRollTrackVariant>;
