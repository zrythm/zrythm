/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * API for Region's specific to instrument Track's.
 */

#ifndef __AUDIO_MIDI_REGION_H__
#define __AUDIO_MIDI_REGION_H__

#include <stdint.h>

typedef struct Track Track;
typedef struct Position Position;
typedef struct MidiNote MidiNote;
typedef struct Region Region;
typedef struct MidiEvents MidiEvents;
typedef Region MidiRegion;
typedef void MIDI_FILE;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Creates a new Region for MIDI notes.
 *
 * @param is_main Is main Region. If this is 1 it
 *   will create the 3 additional Regions (lane,
 *   lane_transient & main_transient).
 */
MidiRegion *
midi_region_new (
  const Position * start_pos,
  const Position * end_pos,
  int        is_main);

/**
 * Adds midi note to region
 */
void
midi_region_add_midi_note (
  MidiRegion * region,
  MidiNote * midi_note);

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
midi_region_pop_unended_note (
  MidiRegion * self,
  int          pitch);

/**
 * Prints the MidiNotes in the Region.
 *
 * Used for debugging.
 */
void
midi_region_print_midi_notes (
  Region * self);

/**
 * Gets first midi note
 */
MidiNote *
midi_region_get_first_midi_note (
	MidiRegion * region);

/**
 * Gets last midi note
 */
MidiNote *
midi_region_get_last_midi_note (
	MidiRegion * region);

/**
 * Gets highest midi note
 */
MidiNote *
midi_region_get_highest_midi_note (
	MidiRegion * region);

/**
 * Gets lowest midi note
 */
MidiNote *
midi_region_get_lowest_midi_note (
	MidiRegion * region);

/**
 * Removes the MIDI note from the Region.
 *
 * @param free Also free the MidiNote.
 * @param pub_event Publish an event.
 */
void
midi_region_remove_midi_note (
  Region *   region,
  MidiNote * midi_note,
  int        free,
  int        pub_event);

/**
 * Removes all MIDI ntoes and their components
 * completely.
 */
void
midi_region_remove_all_midi_notes (
  MidiRegion * region);


/**
 * Returns the midi note at given position with the given
 * pitch.
 *
 * Used when recording.
 */
//MidiNote *
//midi_region_get_midi_note_at (
  //MidiRegion * self,
  //Position *   pos,
  //int          pitch);

/**
 * Exports the Region to an existing MIDI file
 * instance.
 *
 * @param add_region_start Add the region start
 *   offset to the positions.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 */
void
midi_region_write_to_midi_file (
  Region * self,
  MIDI_FILE *    mf,
  const int      add_region_start,
  const int      export_full);

/**
 * Exports the Region to a specified MIDI file.
 *
 * @param full_path Absolute path to the MIDI file.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 */
void
midi_region_export_to_midi_file (
  Region * self,
  const char *   full_path,
  int            midi_version,
  const int      export_full);

/**
 * Returns the MIDI channel that this region should
 * be played on, starting from 1.
 */
uint8_t
midi_region_get_midi_ch (
  const Region * self);

/**
 * Returns a newly initialized MidiEvents with
 * the contents of the region converted into
 * events.
 *
 * Must be free'd with midi_events_free ().
 *
 * @param add_region_start Add the region start
 *   offset to the positions.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 */
MidiEvents *
midi_region_get_as_events (
  Region * self,
  const int      add_region_start,
  const int      full);

/**
 * Frees members only but not the midi region itself.
 *
 * Regions should be free'd using region_free.
 */
void
midi_region_free_members (
  MidiRegion * self);

/**
 * @}
 */

#endif // __AUDIO_MIDI_REGION_H__
