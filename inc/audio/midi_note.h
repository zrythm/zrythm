/*
 * audio/midi_note.h - A midi_note in the timeline having a start
 *   and an end
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#include "audio/position.h"
#include "audio/midi_region.h"

typedef struct _MidiNoteWidget MidiNoteWidget;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct MidiEvents MidiEvents;
typedef struct Position Position;
typedef struct Velocity Velocity;

typedef struct MidiNote
{
  int             id;
  Position        start_pos;
  Position        end_pos;
  MidiNoteWidget  * widget;
  MidiRegion *    midi_region; ///< owner region
  Velocity *      vel;  ///< velocity
  int             val; ///< note
} MidiNote;

MidiNote *
midi_note_new (MidiRegion * region,
               Position *   start_pos,
               Position *   end_pos,
               int          val,
               Velocity *   vel);

void
midi_note_delete (MidiNote * midi_note);

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
 * Converts an array of MIDI notes to MidiEvents.
 */
void
midi_note_notes_to_events (MidiNote     ** midi_notes, ///< array
                           int          num_notes, ///< number of events in array
                           Position     * pos, ///< position to offset time from
                           MidiEvents   * events);  ///< preallocated struct to fill


#endif // __AUDIO_MIDI_NOTE_H__
