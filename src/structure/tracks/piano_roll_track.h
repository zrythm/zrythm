// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "dsp/midi_event.h"
#include "dsp/position.h"
#include "structure/arrangement/midi_region.h"
#include "structure/tracks/laned_track.h"
#include "structure/tracks/recordable_track.h"
#include "utils/types.h"

using MIDI_FILE = void;

#define DEFINE_PIANO_ROLL_TRACK_QML_PROPERTIES(ClassType) \
  DEFINE_RECORDABLE_TRACK_QML_PROPERTIES (ClassType)

namespace zrythm::structure::tracks
{
/**
 * Interface for a piano roll track.
 */
class PianoRollTrack
    : virtual public RecordableTrack,
      public LanedTrackImpl<MidiLane>
{
  Z_DISABLE_COPY_MOVE (PianoRollTrack)
protected:
  PianoRollTrack ();

public:
  ~PianoRollTrack () override = default;

  void init_loaded (
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry) override;

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
  void write_to_midi_file (
    MIDI_FILE *            mf,
    dsp::MidiEventVector * events,
    const Position *       start,
    const Position *       end,
    bool                   lanes_as_tracks,
    bool                   use_track_pos);

  /**
   * Returns the MIDI channel that this region should be played on, starting
   * from 1.
   */
  uint8_t get_midi_ch (const arrangement::MidiRegion &midi_region) const;

  void clear_objects () override;

  void get_regions_in_range (
    std::vector<arrangement::ArrangerObjectUuidReference> &regions,
    std::optional<signed_frame_t>                          p1,
    std::optional<signed_frame_t>                          p2) override;

  bool allow_only_specific_midi_channels () const
  {
    return midi_channels_.has_value ();
  }
  auto &midi_channels_to_allow () const { return midi_channels_.value (); }

protected:
  friend void init_from (
    PianoRollTrack        &obj,
    const PianoRollTrack  &other,
    utils::ObjectCloneType clone_type);

  void set_playback_caches () override;

private:
  static constexpr auto kDrumModeKey = "drumMode"sv;
  static constexpr auto kMidiChKey = "midiCh"sv;
  static constexpr auto kPassthroughMidiInputKey = "passthroughMidiInput"sv;
  static constexpr auto kMidiChannelsKey = "midiChannels"sv;
  friend void           to_json (nlohmann::json &j, const PianoRollTrack &track)
  {
    j[kDrumModeKey] = track.drum_mode_;
    j[kMidiChKey] = track.midi_ch_;
    j[kPassthroughMidiInputKey] = track.passthrough_midi_input_;
    j[kMidiChannelsKey] = track.midi_channels_;
  }
  friend void from_json (const nlohmann::json &j, PianoRollTrack &track)
  {
    j.at (kDrumModeKey).get_to (track.drum_mode_);
    j.at (kMidiChKey).get_to (track.midi_ch_);
    j.at (kPassthroughMidiInputKey).get_to (track.passthrough_midi_input_);
    if (j.contains (kMidiChannelsKey))
      {
        j.at (kMidiChannelsKey).get_to (track.midi_channels_);
      }
  }

private:
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

  /**
   * 1 or 0 flags for each channel to enable it or disable it.
   *
   * If nullopt, the channel will accept MIDI messages from all MIDI channels.
   */
  std::optional<std::array<bool, 16>> midi_channels_;
};

using PianoRollTrackVariant = std::variant<MidiTrack, InstrumentTrack>;
using PianoRollTrackPtrVariant = to_pointer_variant<PianoRollTrackVariant>;
}
