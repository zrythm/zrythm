/*
 * audio/midi_note.c - A midi_note in the timeline having a start
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

#include "audio/midi_note.h"
#include "audio/track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_note.h"

/**
 * default velocity
 */
#define DEFAULT_VEL 90

MidiNote *
midi_note_new (Region * region,
            Position * start_pos,
            Position * end_pos,
            int      val,
            int      vel)
{
  MidiNote * midi_note = calloc (1, sizeof (MidiNote));

  midi_note->start_pos.bars = start_pos->bars;
  midi_note->start_pos.beats = start_pos->beats;
  midi_note->start_pos.quarter_beats = start_pos->quarter_beats;
  midi_note->start_pos.ticks = start_pos->ticks;
  midi_note->end_pos.bars = end_pos->bars;
  midi_note->end_pos.beats = end_pos->beats;
  midi_note->end_pos.quarter_beats = end_pos->quarter_beats;
  midi_note->end_pos.ticks = end_pos->ticks;
  midi_note->region = region;
  midi_note->val = val;
  midi_note->vel = vel > -1 ? vel : DEFAULT_VEL;
  midi_note->widget = midi_note_widget_new (midi_note);
  g_object_ref (midi_note->widget);

  return midi_note;
}

void
midi_note_delete (MidiNote * midi_note)
{
  g_object_unref (midi_note->widget);
  free (midi_note);
}

/**
 * Clamps position then sets it.
 * TODO
 */
void
midi_note_set_start_pos (MidiNote * midi_note,
                      Position * pos)
{
  position_set_to_pos (&midi_note->start_pos,
                       pos);
}

/**
 * Clamps position then sets it.
 */
void
midi_note_set_end_pos (MidiNote * midi_note,
                    Position * pos)
{
  position_set_to_pos (&midi_note->end_pos,
                       pos);
}

