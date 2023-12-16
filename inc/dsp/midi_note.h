// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * API for MIDI notes in the PianoRoll.
 */

#ifndef __AUDIO_MIDI_NOTE_H__
#define __AUDIO_MIDI_NOTE_H__

#include <stdint.h>

#include "dsp/midi_region.h"
#include "dsp/position.h"
#include "dsp/velocity.h"
#include "gui/backend/arranger_object.h"

typedef struct _MidiNoteWidget     MidiNoteWidget;
typedef struct Channel             Channel;
typedef struct Track               Track;
typedef struct MidiEvents          MidiEvents;
typedef struct Position            Position;
typedef struct Velocity            Velocity;
typedef enum PianoRollNoteNotation PianoRollNoteNotation;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define MIDI_NOTE_SCHEMA_VERSION 1

#define MIDI_NOTE_MAGIC 3588791
#define IS_MIDI_NOTE(tr) \
  ((MidiNote *) tr && ((MidiNote *) tr)->magic == MIDI_NOTE_MAGIC)

#define midi_note_is_selected(r) \
  arranger_object_is_selected ((ArrangerObject *) r)

/**
 * A MIDI note inside a ZRegion shown in the piano roll.
 *
 * @extends ArrangerObject
 */
typedef struct MidiNote
{
  /** Base struct. */
  ArrangerObject base;

  /** Velocity. */
  Velocity * vel;

  /** The note/pitch, (0-127). */
  uint8_t val;

  /** Cached note, for live operations. */
  uint8_t cache_val;

  /** Muted or not */
  int muted;

  /** Whether or not this note is currently
   * listened to */
  int currently_listened;

  /** The note/pitch that is currently playing,
   * if \ref MidiNote.currently_listened is true. */
  uint8_t last_listened_val;

  /** Index in the parent region. */
  int pos;

  int magic;

  /** Cache layout for drawing the name. */
  PangoLayout * layout;
} MidiNote;

/**
 * Gets the global Position of the MidiNote's
 * start_pos.
 *
 * @param pos Position to fill in.
 */
void
midi_note_get_global_start_pos (MidiNote * self, Position * pos);

/**
 * Creates a new MidiNote.
 */
MidiNote *
midi_note_new (
  RegionIdentifier * region_id,
  Position *         start_pos,
  Position *         end_pos,
  uint8_t            val,
  uint8_t            vel);

/**
 * Sets the region the MidiNote belongs to.
 */
void
midi_note_set_region_and_index (MidiNote * self, ZRegion * region, int idx);

void
midi_note_set_cache_val (MidiNote * self, const uint8_t val);

/**
 * Returns 1 if the MidiNotes match, 0 if not.
 */
NONNULL PURE int
midi_note_is_equal (MidiNote * src, MidiNote * dest);

/**
 * Gets the MIDI note's value as a string (eg "C#4").
 *
 * @param use_markup Use markup to show the octave as a
 *   superscript.
 */
void
midi_note_get_val_as_string (
  const MidiNote *      self,
  char *                buf,
  PianoRollNoteNotation notation,
  const int             use_markup);

/**
 * For debugging.
 */
void
midi_note_print (MidiNote * mn);

/**
 * Listen to the given MidiNote.
 *
 * @param listen Turn note on if 1, or turn it
 *   off if 0.
 */
void
midi_note_listen (MidiNote * mn, bool listen);

/**
 * Shifts MidiNote's position and/or value.
 *
 * @param delta Y (0-127)
 */
void
midi_note_shift_pitch (MidiNote * self, const int delta);

/**
 * Returns if the MIDI note is hit at given pos (in
 * the timeline).
 */
int
midi_note_hit (MidiNote * self, const signed_frame_t gframes);

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
midi_note_set_val (MidiNote * midi_note, const uint8_t val);

ZRegion *
midi_note_get_region (MidiNote * self);

/**
 * @}
 */

#endif // __AUDIO_MIDI_NOTE_H__
