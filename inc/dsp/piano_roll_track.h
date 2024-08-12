// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_PIANO_ROLL_TRACK_H__
#define __DSP_PIANO_ROLL_TRACK_H__

#include <vector>

#include "dsp/laned_track.h"
#include "dsp/midi_region.h"
#include "dsp/recordable_track.h"
#include "utils/types.h"

class Position;
class MidiEvents;
class Velocity;
using MIDI_FILE = void;

/**
 * Interface for a piano roll track.
 */
class PianoRollTrack
    : virtual public RecordableTrack,
      virtual public LanedTrackImpl<MidiRegion>,
      public ISerializable<PianoRollTrack>
{
public:
  // Rule of 0

  virtual ~PianoRollTrack () = default;

  void init_loaded () override
  {
    RecordableTrack::init_loaded ();
    LanedTrackImpl::init_loaded ();
  }

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

  /**
   * Wrapper for MIDI/instrument tracks to fill in MidiEvents from the timeline
   * data.
   *
   * @note The engine splits the cycle so transport loop related logic is not
   * needed.
   *
   * @param midi_events MidiEvents to fill.
   */
  void fill_events (
    const EngineProcessTimeInfo &time_nfo,
    MidiEventVector             &midi_events);

  void clear_objects () override
  {
    LanedTrackImpl::clear_objects ();
    AutomatableTrack::clear_objects ();
  }

  bool validate () const override
  {
    return AutomatableTrack::validate () && PianoRollTrack::validate ();
  }

  void get_regions_in_range (
    std::vector<Region *> &regions,
    const Position *       p1,
    const Position *       p2) override
  {
    LanedTrackImpl::get_regions_in_range (regions, p1, p2);
    AutomatableTrack::get_regions_in_range (regions, p1, p2);
  }

protected:
  void copy_members_from (const PianoRollTrack &other)
  {
    drum_mode_ = other.drum_mode_;
    midi_ch_ = other.midi_ch_;
    passthrough_midi_input_ = other.passthrough_midi_input_;
  }

  void set_playback_caches () override
  {
    LanedTrackImpl::set_playback_caches ();
    AutomatableTrack::set_playback_caches ();
  }

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

private:
  void update_name_hash (unsigned int new_name_hash) override
  {
    AutomatableTrack::update_name_hash (new_name_hash);
    LanedTrackImpl::update_name_hash (new_name_hash);
  }

public:
  /**
   * Whether drum mode in the piano roll is enabled for this track.
   */
  bool drum_mode_ = false;

  /** MIDI channel. FIXME specify if 0-15 or 1-16 */
  midi_byte_t midi_ch_ = 0;

  /**
   * If true, the input received will not be changed to the selected MIDI
   * channel.
   *
   * If false, all input received will have its channel changed to the
   * selected MIDI channel.
   */
  bool passthrough_midi_input_ = false;
};

#endif // __DSP_PIANO_ROLL_TRACK_H__
