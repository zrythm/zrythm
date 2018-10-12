/*
 * audio/midi_note.h - A midi_note in the timeline having a start
 *   and an end
 *
 * Copyright (C) 2018 Alexandros Theodotou
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
#include "audio/region.h"

typedef struct MidiNoteWidget MidiNoteWidget;
typedef struct Channel Channel;
typedef struct Track Track;

typedef struct MidiNote
{
  Position        start_pos;
  Position        end_pos;
  MidiNoteWidget  * widget;
  Region          * region; ///< owner region
  int             vel;  ///< velocity
  int             val; ///< note
} MidiNote;

MidiNote *
midi_note_new (Region * track,
               Position * start_pos,
               Position * end_pos,
               int      val,
               int      vel);

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

#endif // __AUDIO_POSITION_H__

