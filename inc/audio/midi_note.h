/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_MIDI_NOTE_H__
#define __AUDIO_MIDI_NOTE_H__

#include "audio/midi_region.h"
#include "audio/position.h"
#include "audio/velocity.h"

typedef struct _MidiNoteWidget MidiNoteWidget;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct MidiEvents MidiEvents;
typedef struct Position Position;
typedef struct Velocity Velocity;

typedef enum MidiNoteCloneFlag
{
  /**
   * Create a completely new region with a new id.
   */
  MIDI_NOTE_CLONE_COPY,
  MIDI_NOTE_CLONE_LINK
} MidiNoteCloneFlag;

typedef struct MidiNote
{
  Position        start_pos;
  Position        end_pos;
  MidiNoteWidget  * widget;

  /**
   * Owner.
   *
   * For convenience only (cache).
   */
  Region *        region; ///< cache
  char *          region_name;

  Velocity *      vel;  ///< velocity
  int             val; ///< note

  /** Muted or not */
  int             muted;

  /**
   * Transient or not.
   *
   * Transient notes are notes that are cloned
   * and used during moving, then discarded.  */
  int             transient;
} MidiNote;

static const cyaml_schema_field_t
  midi_note_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "start_pos",
    /* direct struct inside struct -> default */
    CYAML_FLAG_DEFAULT,
    MidiNote, start_pos, position_fields_schema),
  CYAML_FIELD_MAPPING (
    "end_pos", CYAML_FLAG_DEFAULT,
    MidiNote, end_pos, position_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "vel", CYAML_FLAG_POINTER,
    MidiNote, vel, velocity_fields_schema),
	CYAML_FIELD_INT (
    "val", CYAML_FLAG_DEFAULT,
    MidiNote, val),
	CYAML_FIELD_INT (
    "muted", CYAML_FLAG_DEFAULT,
    MidiNote, muted),
  CYAML_FIELD_STRING_PTR (
    "region_name", CYAML_FLAG_POINTER,
    MidiNote, region_name,
   	0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
midi_note_schema = {
    /* wherever it is used it will be a pointer */
	CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
			MidiNote, midi_note_fields_schema),
};

void
midi_note_init_loaded (
  MidiNote * self);

/**
 */
MidiNote *
midi_note_new (MidiRegion * region,
               Position *   start_pos,
               Position *   end_pos,
               int          val,
               Velocity *   vel);

/**
 * Finds the actual MidiNote in the project from the
 * given clone.
 */
MidiNote *
midi_note_find (
  MidiNote * clone);

/**
 * Deep clones the midi note.
 */
MidiNote *
midi_note_clone (MidiNote *  src);

/**
 * Returns 1 if the MidiNotes match, 0 if not.
 */
int
midi_note_is_equal (
  MidiNote * src,
  MidiNote * dest);

void
midi_note_delete (MidiNote * midi_note);

/**
 * Resizes the MidiNote on the left side or right side
 * by given amount of ticks.
 *
 * @param left 1 to resize left side, 0 to resize right
 *   side.
 * @param ticks Number of ticks to resize.
 */
void
midi_note_resize (
  MidiNote * r,
  int      left,
  long     ticks);

/**
 * For debugging.
 */
void
midi_note_print (
  MidiNote * mn);

/**
 * Returns if MidiNote is in MidiArrangerSelections.
 */
int
midi_note_is_selected (MidiNote * self);

/**
 * Returns if MidiNote is (should be) visible.
 */
int
midi_note_is_visible (MidiNote * self);

/**
 * Checks if position is valid then sets it.
 */
void
midi_note_set_start_pos (MidiNote * midi_note,
                         Position * start_pos);

/**
 * Checks if position is valid then sets it.
 */
void
midi_note_set_end_pos (MidiNote * midi_note,
                       Position * end_pos);

/**
 * Sets note's visible flag and makes its widget
 * visible or not.
 */
//void
//midi_note_set_visible (
  //MidiNote * midi_note,
  //int        visible);

/**
 * Shifts MidiNote's position and/or value.
 */
void
midi_note_shift (
  MidiNote * self,
  long       ticks, ///< x (Position)
  int        delta); ///< y (0-127)

/**
 * Returns if the MIDI note is hit at given pos (in the
 * timeline).
 */
int
midi_note_hit (MidiNote * midi_note,
               Position *  pos);

/**
 * Converts an array of MIDI notes to MidiEvents.
 */
void
midi_note_notes_to_events (MidiNote     ** midi_notes, ///< array
                           int          num_notes, ///< number of events in array
                           Position     * pos, ///< position to offset time from
                           MidiEvents   * events);  ///< preallocated struct to fill

void
midi_note_free (MidiNote * self);

#endif // __AUDIO_MIDI_NOTE_H__
