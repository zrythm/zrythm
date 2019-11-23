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

#include <stdint.h>

#include "audio/midi_region.h"
#include "audio/position.h"
#include "audio/velocity.h"
#include "gui/backend/arranger_object.h"

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

#define midi_note_is_selected(r) \
  arranger_object_is_selected ( \
    (ArrangerObject *) r)

/**
 * A MIDI note inside a Region shown in the
 * piano roll.
 */
typedef struct MidiNote
{
  /** Base struct. */
  ArrangerObject  base;

  /** Owner region (cache). */
  Region *        region;
  char *          region_name;

  /** Velocity. */
  Velocity *      vel;

  /** The note/pitch, (0-127). */
  uint8_t         val;

  /** Cached note, for live operations. */
  uint8_t         cache_val;

  /** Muted or not */
  int             muted;

  /** Cache layout for drawing the name. */
  PangoLayout *   layout;
} MidiNote;

static const cyaml_schema_field_t
  midi_note_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "base", CYAML_FLAG_DEFAULT,
    MidiNote, base,
    arranger_object_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "vel", CYAML_FLAG_POINTER,
    MidiNote, vel, velocity_fields_schema),
	CYAML_FIELD_UINT (
    "val", CYAML_FLAG_DEFAULT,
    MidiNote, val),
	CYAML_FIELD_INT (
    "muted", CYAML_FLAG_DEFAULT,
    MidiNote, muted),
  CYAML_FIELD_STRING_PTR (
    "region_name",
    CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
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
  uint8_t      val,
  uint8_t      vel,
  int          is_main);

/**
 * Sets the Region the MidiNote belongs to.
 */
void
midi_note_set_region (
  MidiNote * midi_note,
  Region *   region);

void
midi_note_set_cache_val (
  MidiNote *    self,
  const uint8_t val);

/**
 * Returns 1 if the MidiNotes match, 0 if not.
 */
int
midi_note_is_equal (
  MidiNote * src,
  MidiNote * dest);

/**
 * Gets the MIDI note's value as a string (eg
 * "C#4").
 *
 * @param use_markup Use markup to show the octave
 *   as a superscript.
 */
void
midi_note_get_val_as_string (
  MidiNote * self,
  char *     buf,
  const int  use_markup);

/**
 * For debugging.
 */
void
midi_note_print (
  MidiNote * mn);

/**
 * Shifts MidiNote's position and/or value.
 *
 * @param delta Y (0-127)
 */
void
midi_note_shift_pitch (
  MidiNote * self,
  const int  delta);

/**
 * Returns if the MIDI note is hit at given pos (in
 * the timeline).
 */
int
midi_note_hit (
  MidiNote * midi_note,
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
 * Sends a note off if currently playing and sets
 * the pitch of the MidiNote.
 */
void
midi_note_set_val (
  MidiNote *    midi_note,
  const uint8_t val);

/**
 * @}
 */

#endif // __AUDIO_MIDI_NOTE_H__
