/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * API for MIDI notes in the PianoRoll.
 */

#ifndef __AUDIO_MIDI_NOTE_H__
#define __AUDIO_MIDI_NOTE_H__

#include "audio/midi_region.h"
#include "audio/position.h"
#include "audio/velocity.h"
#include "gui/backend/arranger_object.h"
#include "gui/backend/arranger_object_info.h"

typedef struct _MidiNoteWidget MidiNoteWidget;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct MidiEvents MidiEvents;
typedef struct Position Position;
typedef struct Velocity Velocity;

/**
 * @addtogroup audio
 *
 * @{
 */

#define midi_note_is_main(r) \
  arranger_object_info_is_main ( \
    &r->obj_info)

#define midi_note_is_transient(r) \
  arranger_object_info_is_transient ( \
    &r->obj_info)

/** Gets the transient counterpart of the
 * MidiNote. */
#define midi_note_get_main_trans_midi_note(r) \
  ((MidiNote *) r->obj_info.main_trans)

/** Gets the main counterpart of the
 * MidiNote. */
#define midi_note_get_main_midi_note(r) \
  ((MidiNote *) r->obj_info.main)

typedef enum MidiNoteCloneFlag
{
  /** Create a new MidiNote to be added to a
   * Region as a main MidiNote. */
  MIDI_NOTE_CLONE_COPY_MAIN,

  /** Create a new MidiNote that will not be used
   * as a main MidiNote. */
  MIDI_NOTE_CLONE_COPY,
  MIDI_NOTE_CLONE_LINK
} MidiNoteCloneFlag;

/**
 * A MIDI note living inside a Region and shown on the
 * piano roll.
 */
typedef struct MidiNote
{
  /** Start Position. */
  Position        start_pos;

  /** Cache start Position. */
  Position        cache_start_pos;

  /** End Position. */
  Position        end_pos;

  /** Cached end Position, for live operations. */
  Position        cache_end_pos;

  /** GUI widget. */
  MidiNoteWidget  * widget;

  /**
   * Owner.
   *
   * For convenience only (cache).
   */
  Region *        region; ///< cache
  char *          region_name;

  Velocity *      vel;  ///< velocity

  /** The note/pitch, (0-127). */
  int             val;

  /** Cached note, for live operations. */
  int             cache_val;

  /** Muted or not */
  int             muted;

  /**
   * Info on whether this MidiNote is transient/lane
   * and pointers to transient/lane equivalents.
   */
  ArrangerObjectInfo obj_info;
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
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    MidiNote, midi_note_fields_schema),
};

ARRANGER_OBJ_DECLARE_MOVABLE_W_LENGTH (
  MidiNote, midi_note);

void
midi_note_init_loaded (
  MidiNote * self);

/**
 * Gets the global Position of the MidiNote's
 * start_pos.
 *
 * @param pos Position to fill in.
 */
void
midi_note_get_global_start_pos (
  MidiNote * self,
  Position * pos);

/**
 * @param is_main Is main MidiNote. If this is 1 then
 *   arranger_object_info_init_main() is called to
 *   create a transient midi note in obj_info.
 */
MidiNote *
midi_note_new (
  MidiRegion * region,
  Position *   start_pos,
  Position *   end_pos,
  int          val,
  int          vel,
  int          is_main);

/**
 * Sets the Region the MidiNote belongs to.
 */
void
midi_note_set_region (
  MidiNote * midi_note,
  Region *   region);

/**
 * Finds the actual MidiNote in the project from the
 * given clone.
 */
MidiNote *
midi_note_find (
  MidiNote * clone);

/**
 * Gets the Track this MidiNote is in.
 */
Track *
midi_note_get_track (
  MidiNote * self);

/**
 * Deep clones the midi note.
 */
MidiNote *
midi_note_clone (
  MidiNote *  src,
  MidiNoteCloneFlag flag);

/**
 * Updates the frames of each position in each child
 * of the MidiNote recursively.
 */
void
midi_note_update_frames (
  MidiNote * self);

void
midi_note_set_cache_val (
  MidiNote * self,
  int        val);

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
 * For debugging.
 */
void
midi_note_print (
  MidiNote * mn);

/**
 * Returns if MidiNote is (should be) visible.
 */
#define midi_note_should_be_visible(mn) \
  arranger_object_info_should_be_visible ( \
    mn->obj_info)

/**
 * Shifts MidiNote's position and/or value.
 *
 * @param delta Y (0-127)
 */
void
midi_note_shift_pitch (
  MidiNote * self,
  int        delta,
  ArrangerObjectUpdateFlag update_flag);

/**
 * Returns if the MIDI note is hit at given pos (in
 * the timeline).
 */
int
midi_note_hit (
  const MidiNote * midi_note,
  const long       gframes);

/**
 * Converts an array of MIDI notes to MidiEvents.
 *
 * @param midi_notes Array of MidiNote's.
 * @param num_notes Number of notes in array.
 * @param pos Position to offset time from.
 * @param events Preallocated struct to fill.
 */
void
midi_note_notes_to_events (
  MidiNote **  midi_notes,
  int          num_notes,
  Position *   pos,
  MidiEvents * events);

/**
 * Returns the MidiNote length in ticks.
 */
int
midi_note_get_length_in_ticks (
  const MidiNote * self);

#define midi_note_reset_transient(mn) \
  midi_note_reset_counterpart (mn, 1)

/**
 * Sets the pitch of the MidiNote.
 */
void
midi_note_set_val (
  MidiNote * midi_note,
  int        val,
  ArrangerObjectUpdateFlag update_flag);

ARRANGER_OBJ_DECLARE_FREE_ALL_LANELESS (
  MidiNote, midi_note);

/**
 * Frees a single MidiNote and its components.
 */
void
midi_note_free (MidiNote * self);

/**
 * @}
 */

#endif // __AUDIO_MIDI_NOTE_H__
