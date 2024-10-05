// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MIDI_REGION_H__
#define __AUDIO_MIDI_REGION_H__

#include "common/dsp/lane_owned_object.h"
#include "common/dsp/midi_note.h"
#include "common/dsp/region.h"

class Track;
class Position;
class MidiNote;
class MidiEventVector;
class ChordDescriptor;
class Velocity;
using MIDI_FILE = void;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * @brief A Region containing MIDI events.
 *
 * MidiRegion represents a region in the timeline that holds MIDI note and
 * controller data. It is specific to instrument/MIDI tracks and can be
 * constructed from a MIDI file or a chord descriptor.
 */
class MidiRegion final
    : public LaneOwnedObjectImpl<MidiRegion>,
      public RegionImpl<MidiRegion>,
      public ICloneable<MidiRegion>,
      public ISerializable<MidiRegion>
{
public:
  MidiRegion ();

  /**
   * @brief Construct a new Midi Region object
   *
   * @param start_pos
   * @param end_pos
   * @param track_name_hash
   * @param lane_pos
   * @param idx_inside_lane
   */
  MidiRegion (
    const Position &start_pos,
    const Position &end_pos,
    unsigned int    track_name_hash,
    int             lane_pos,
    int             idx_inside_lane);

  /**
   * Creates a MIDI region from the given MIDI file path, starting at the given
   * Position.
   *
   * @param midi_track_idx The index of this track, starting from 0. This will
   * be sequential, ie, if idx 1 is requested and the MIDI file only has tracks
   * 5 and 7, it will use track 7.
   *
   * @throw ZrythmException on I/O error.
   */
  MidiRegion (
    const Position    &start_pos,
    const std::string &abs_path,
    unsigned int       track_name_hash,
    int                lane_pos,
    int                idx_inside_lane,
    int                midi_track_idx);

  /**
   * Create a region from the chord descriptor.
   *
   * Default size will be timeline snap and default
   * notes size will be editor snap.
   */
  MidiRegion (
    const Position  &pos,
    ChordDescriptor &descr,
    unsigned int     track_name_hash,
    int              lane_pos,
    int              idx_inside_lane);

  void init_loaded () override;

  /**
   * Starts an unended note with the given pitch and velocity and adds it to
   * @ref midi_notes_.
   *
   * @param end_pos If this is nullptr, it will be set to 1 tick after the
   * start_pos.
   */
  void start_unended_note (
    Position * start_pos,
    Position * end_pos,
    int        pitch,
    int        vel,
    bool       pub_events);

  /**
   * Returns the midi note with the given pitch from the unended notes.
   *
   * Used when recording.
   *
   * @param pitch The pitch. If -1, it returns any unended note. This is useful
   * when the loop point is met and we want to end them all.
   */
  MidiNote * pop_unended_note (int pitch);

  /**
   * Prints the MidiNotes in the Region.
   *
   * Used for debugging.
   */
  void print_midi_notes () const;

  /**
   * Gets first midi note
   */
  MidiNote * get_first_midi_note ();

  /**
   * Gets last midi note
   */
  MidiNote * get_last_midi_note ();

  /**
   * Gets highest midi note
   */
  MidiNote * get_highest_midi_note ();

  /**
   * Gets lowest midi note
   */
  MidiNote * get_lowest_midi_note ();

  /**
   * Exports the Region to an existing MIDI file instance.
   *
   * This must only be called when exporting single regions.
   *
   * @param add_region_start Add the region start offset to the positions.
   * @param export_full Traverse loops and export the MIDI file as it would be
   * played inside Zrythm. If this is 0, only the original region (from true
   * start to true end) is exported.
   * @param lanes_as_tracks Export lanes as separate tracks (only possible with
   * MIDI type 1). This will calculate a unique MIDI track number for the
   * region's lane.
   * @param use_track_or_lane_pos Whether to use the track/lane position in the
   * MIDI data. The MIDI track will be set to 1 if false.
   */
  void write_to_midi_file (
    MIDI_FILE * mf,
    const bool  add_region_start,
    bool        export_full) const;

  /**
   * Exports the Region to a specified MIDI file.
   *
   * @param full_path Absolute path to the MIDI file.
   * @param export_full Traverse loops and export the
   *   MIDI file as it would be played inside Zrythm.
   *   If this is 0, only the original region (from
   *   true start to true end) is exported.
   */
  void export_to_midi_file (
    const std::string &full_path,
    int                midi_version,
    const bool         export_full) const;

  /**
   * Returns the MIDI channel that this region should be played on, starting
   * from 1.
   */
  uint8_t get_midi_ch () const;

  /**
   * Returns whether the given note is not muted and starts within any
   * playable part of the region.
   */
  bool is_note_playable (const MidiNote &midi_note) const;

  /**
   * Adds the contents of the region converted into events.
   *
   * @param add_region_start Add the region start offset to the
   *   positions.
   * @param export_full Traverse loops and export the MIDI file
   *   as it would be played inside Zrythm. If this is 0, only
   *   the original region (from true start to true end) is
   *   exported.
   * @param start Events before this (global) position will be skipped.
   * @param end Events after this (global) position will be skipped.
   */
  void add_events (
    MidiEventVector &events,
    const Position * start,
    const Position * end,
    const bool       add_region_start,
    const bool       full) const;

  /**
   * Fills in the array with all the velocities in the project that are within
   * or outside the range given.
   *
   * @param inside Whether to find velocities inside the range (true) or
   * outside (false).
   */
  void get_velocities_in_range (
    const Position *         start_pos,
    const Position *         end_pos,
    std::vector<Velocity *> &velocities,
    bool                     inside);

  /**
   * @copydoc ArrangerObject::print_to_str()
   */
  std::string print_to_str () const override;

  void append_children (
    std::vector<RegionOwnedObjectImpl<MidiRegion> *> &children) const override
  {
    for (const auto &mn : midi_notes_)
      {
        children.push_back (mn.get ());
      }
  }

  void add_ticks_to_children (const double ticks) override
  {
    for (auto &mn : midi_notes_)
      {
        mn->move (ticks);
      }
  }

  bool validate (bool is_project, double frames_per_tick) const override;

  ArrangerSelections * get_arranger_selections () const override;
  ArrangerWidget *     get_arranger_for_children () const override;

  void init_after_cloning (const MidiRegion &other) override
  {
    clone_unique_ptr_container (midi_notes_, other.midi_notes_);
    LaneOwnedObjectImpl::copy_members_from (other);
    Region::copy_members_from (other);
    TimelineObject::copy_members_from (other);
    NameableObject::copy_members_from (other);
    LoopableObject::copy_members_from (other);
    MuteableObject::copy_members_from (other);
    LengthableObject::copy_members_from (other);
    ColoredObject::copy_members_from (other);
    ArrangerObject::copy_members_from (other);
  }

  /**
   * Set positions to the exact values in the export region as it is played
   * inside Zrythm.
   *
   * @param[in,out] start_pos start position of the event
   * @param[in,out] end_pos end position of the event
   * @param repeat_index repetition counter for loop offset
   */
  void get_note_positions_in_export (
    Position * start_pos,
    Position * end_pos,
    int        repeat_index) const;

  /**
   * Returns if the given positions are in a given
   * region as it is played inside Zrythm.
   *
   * @param offset_in_ticks Offset value if note is
   * repeated inside a loop
   */
  bool is_note_export_start_pos_in_full_region (Position start_pos) const;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /**
   * MIDI notes.
   */
  std::vector<std::shared_ptr<MidiNote>> midi_notes_;

  /**
   * Unended notes started in recording with MIDI NOTE ON signal but haven't
   * received a NOTE OFF yet.
   *
   * This is also used temporarily when reading from MIDI files.
   *
   * @note These are present in @ref midi_notes_ and must not be deleted
   * separately.
   */
  std::vector<MidiNote *> unended_notes_;
};

inline bool
operator== (const MidiRegion &lhs, const MidiRegion &rhs)
{
  return static_cast<const Region &> (lhs) == static_cast<const Region &> (rhs)
         && static_cast<const TimelineObject &> (lhs)
              == static_cast<const TimelineObject &> (rhs)
         && static_cast<const NameableObject &> (lhs)
              == static_cast<const NameableObject &> (rhs)
         && static_cast<const LoopableObject &> (lhs)
              == static_cast<const LoopableObject &> (rhs)
         && static_cast<const ColoredObject &> (lhs)
              == static_cast<const ColoredObject &> (rhs)
         && static_cast<const MuteableObject &> (lhs)
              == static_cast<const MuteableObject &> (rhs)
         && static_cast<const LengthableObject &> (lhs)
              == static_cast<const LengthableObject &> (rhs)
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs);
}

DEFINE_OBJECT_FORMATTER (MidiRegion, [] (const MidiRegion &mr) {
  return fmt::format ("MidiRegion[id: {}]", mr.id_);
})

/**
 * @}
 */

#endif // __AUDIO_MIDI_REGION_H__
