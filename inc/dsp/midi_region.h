// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * API for Region's specific to instrument Track's.
 */

#ifndef __AUDIO_MIDI_REGION_H__
#define __AUDIO_MIDI_REGION_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "utils/types.h"

typedef struct Track           Track;
typedef struct Position        Position;
typedef struct MidiNote        MidiNote;
typedef struct ZRegion         ZRegion;
typedef struct MidiEvents      MidiEvents;
typedef struct ChordDescriptor ChordDescriptor;
typedef struct Velocity        Velocity;
typedef void                   MIDI_FILE;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Creates a new ZRegion for MIDI notes.
 */
ZRegion *
midi_region_new (
  const Position * start_pos,
  const Position * end_pos,
  unsigned int     track_name_hash,
  int              lane_pos,
  int              idx_inside_lane);

/**
 * Creates a MIDI region from the given MIDI
 * file path, starting at the given Position.
 *
 * @param idx The index of this track, starting from
 *   0. This will be sequential, ie, if idx 1 is
 *   requested and the MIDI file only has tracks
 *   5 and 7, it will use track 7.
 */
ZRegion *
midi_region_new_from_midi_file (
  const Position * start_pos,
  const char *     abs_path,
  unsigned int     track_name_hash,
  int              lane_pos,
  int              idx_inside_lane,
  int              idx);

/**
 * Create a region from the chord descriptor.
 *
 * Default size will be timeline snap and default
 * notes size will be editor snap.
 */
ZRegion *
midi_region_new_from_chord_descr (
  const Position *  pos,
  ChordDescriptor * descr,
  unsigned int      track_name_hash,
  int               lane_pos,
  int               idx_inside_lane);

/**
 * Adds the MidiNote to the given ZRegion.
 *
 * @param pub_events Publish UI events or not.
 */
#define midi_region_add_midi_note(region, midi_note, pub_events) \
  midi_region_insert_midi_note ( \
    region, midi_note, ((ZRegion *) (region))->num_midi_notes, pub_events)

/**
 * Inserts the MidiNote to the given ZRegion.
 *
 * @param idx Index to insert at.
 * @param pub_events Publish UI events or not.
 */
void
midi_region_insert_midi_note (
  ZRegion *  region,
  MidiNote * midi_note,
  int        idx,
  int        pub_events);

/**
 * Starts an unended note with the given pitch and
 * velocity and adds it to \ref ZRegion.midi_notes.
 *
 * @param end_pos If this is NULL, it will be set to
 *   1 tick after the start_pos.
 */
void
midi_region_start_unended_note (
  ZRegion *  self,
  Position * start_pos,
  Position * end_pos,
  int        pitch,
  int        vel,
  int        pub_events);

/**
 * Returns the midi note with the given pitch from
 * the unended notes.
 *
 * Used when recording.
 *
 * @param pitch The pitch. If -1, it returns any
 *   unended note. This is useful when the loop
 *   point is met and we want to end them all.
 */
MidiNote *
midi_region_pop_unended_note (ZRegion * self, int pitch);

/**
 * Fills MIDI event queue from the region.
 *
 * The events are dequeued right after the call to
 * this function.
 *
 * @note The caller already splits calls to this
 *   function at each sub-loop inside the region,
 *   so region loop related logic is not needed.
 *
 * @param note_off_at_end Whether a note off should
 *   be added at the end frame (eg, when the caller
 *   knows there is a region loop or the region
 *   ends).
 * @param midi_events MidiEvents to fill (from
 *   Piano Roll Port for example).
 */
OPTIMIZE_O3
REALTIME
void
midi_region_fill_midi_events (
  ZRegion *                           self,
  const EngineProcessTimeInfo * const time_nfo,
  bool                                note_off_at_end,
  MidiEvents *                        midi_events);

/**
 * Prints the MidiNotes in the Region.
 *
 * Used for debugging.
 */
void
midi_region_print_midi_notes (ZRegion * self);

/**
 * Gets first midi note
 */
MidiNote *
midi_region_get_first_midi_note (ZRegion * region);

/**
 * Gets last midi note
 */
MidiNote *
midi_region_get_last_midi_note (ZRegion * region);

/**
 * Gets highest midi note
 */
MidiNote *
midi_region_get_highest_midi_note (ZRegion * region);

/**
 * Gets lowest midi note
 */
MidiNote *
midi_region_get_lowest_midi_note (ZRegion * region);

/**
 * Removes the MIDI note from the Region.
 *
 * @param free Also free the MidiNote.
 * @param pub_event Publish an event.
 */
void
midi_region_remove_midi_note (
  ZRegion *  region,
  MidiNote * midi_note,
  int        free,
  int        pub_event);

/**
 * Removes all MIDI ntoes and their components
 * completely.
 */
void
midi_region_remove_all_midi_notes (ZRegion * region);

/**
 * Returns the midi note at given position with the given
 * pitch.
 *
 * Used when recording.
 */
//MidiNote *
//midi_region_get_midi_note_at (
//ZRegion * self,
//Position *   pos,
//int          pitch);

/**
 * Exports the ZRegion to an existing MIDI file
 * instance.
 *
 * This must only be called when exporting single
 * regions.
 *
 * @param add_region_start Add the region start
 *   offset to the positions.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 * @param lanes_as_tracks Export lanes as separate
 *   tracks (only possible with MIDI type 1). This
 *   will calculate a unique MIDI track number for
 *   the region's lane.
 * @param use_track_or_lane_pos Whether to use the
 *   track/lane position in the MIDI data. The
 *   MIDI track will be set to 1 if false.
 */
NONNULL_ARGS (1, 2)
void midi_region_write_to_midi_file (
  const ZRegion * self,
  MIDI_FILE *     mf,
  const bool      add_region_start,
  bool            export_full);

/**
 * Exports the ZRegion to a specified MIDI file.
 *
 * @param full_path Absolute path to the MIDI file.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 */
NONNULL void
midi_region_export_to_midi_file (
  const ZRegion * self,
  const char *    full_path,
  int             midi_version,
  const bool      export_full);

/**
 * Returns the MIDI channel that this region should
 * be played on, starting from 1.
 */
uint8_t
midi_region_get_midi_ch (const ZRegion * self);

/**
 * Returns whether the given note is not muted and starts within any
 * playable part of the region.
 */
bool
midi_region_is_note_playable (const ZRegion * self, const MidiNote * midi_note);

/**
 * Adds the contents of the region converted into events.
 *
 * @param add_region_start Add the region start offset to the
 *   positions.
 * @param export_full Traverse loops and export the MIDI file
 *   as it would be played inside Zrythm. If this is 0, only
 *   the original region (from true start to true end) is
 *   exported.
 * @param start Events before this (global) position will be
 *   skipped.
 * @param end Events after this (global) position will be
 *   skipped.
 */
void
midi_region_add_events (
  const ZRegion *  self,
  MidiEvents *     events,
  const Position * start,
  const Position * end,
  const bool       add_region_start,
  const bool       full);

/**
 * Fills in the array with all the velocities in
 * the project that are within or outside the
 * range given.
 *
 * @param inside Whether to find velocities inside
 *   the range (1) or outside (0).
 */
void
midi_region_get_velocities_in_range (
  const ZRegion *  self,
  const Position * start_pos,
  const Position * end_pos,
  Velocity ***     velocities,
  int *            num_velocities,
  size_t *         velocities_size,
  int              inside);

/**
 * Frees members only but not the midi region itself.
 *
 * Regions should be free'd using region_free().
 */
void
midi_region_free_members (ZRegion * self);

/**
 * @}
 */

#endif // __AUDIO_MIDI_REGION_H__
