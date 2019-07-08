/*
 * Copyright (C) 2019 Alexandros Theodotou
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

typedef struct Track Track;
typedef struct Position Position;
typedef struct MidiNote MidiNote;
typedef struct Region Region;
typedef Region MidiRegion;

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
 * Creates region (used when loading projects).
 */
MidiRegion *
midi_region_get_or_create_blank (int id);

/**
 * Adds midi note to region
 *
 * @param gen_widget Generate a MidiNoteWidget.
 */
void
midi_region_add_midi_note (
  MidiRegion * region,
  MidiNote * midi_note,
  int        gen_widget);

/**
 * Returns the midi note with the given pitch from the
 * unended notes.
 *
 * Used when recording.
 */
MidiNote *
midi_region_find_unended_note (MidiRegion * self,
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
 * Frees members only but not the midi region itself.
 *
 * Regions should be free'd using region_free.
 */
void
midi_region_free_members (MidiRegion * self);

/**
 * @}
 */

#endif // __AUDIO_MIDI_REGION_H__
