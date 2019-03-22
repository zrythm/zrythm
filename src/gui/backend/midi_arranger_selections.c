/*
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/midi_arranger_selections.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

/**
 * Returns the position of the leftmost object.
 */
static void
get_start_pos (
  MidiArrangerSelections * ts,
  Position *           pos) ///< position to fill in
{
  position_set_to_bar (pos,
                       TRANSPORT->total_bars);

  for (int i = 0; i < ts->num_midi_notes; i++)
    {
      MidiNote * midi_note = ts->midi_notes[i];
      if (position_compare (&midi_note->start_pos,
                            pos) < 0)
        position_set_to_pos (pos,
                             &midi_note->start_pos);
    }
}

/**
 * Shift note val for selected noted
 */
void
midi_arranger_selections_shift_val (int delta)
{

	MidiArrangerSelections * src =
	MIDI_ARRANGER_SELECTIONS;
	for (int i = 0; 
		i < src->num_midi_notes; 
		i++)
	{
		src->midi_notes[i]->val = 
			src->midi_notes[i]->val + delta;
	}
}

/**
 * Shift note pos for selected noted
 */
void
midi_arranger_selections_shift_pos (int delta)
{

	MidiArrangerSelections * src =
	MIDI_ARRANGER_SELECTIONS;
	for (int i = 0; 
		i < src->num_midi_notes; 
		i++)
	{
		position_add_beats(
			&src->midi_notes[i]->start_pos,
			delta);
		position_add_beats(
			&src->midi_notes[i]->end_pos,
			delta);
	}
}

/**
 * Clone the struct for copying, undoing, etc.
 */
MidiArrangerSelections *
midi_arranger_selections_clone ()
{
  MidiArrangerSelections * new_ts =
    calloc (1, sizeof (MidiArrangerSelections));

  MidiArrangerSelections * src =
    MIDI_ARRANGER_SELECTIONS;
  for (int i = 0; i < src->num_midi_notes; i++)
    {
      MidiNote * r = src->midi_notes[i];
      MidiNote * new_r =
        midi_note_clone (r, r->midi_region);
     position_add_beats(&new_r->start_pos,1);
     position_add_beats(&new_r->end_pos,1);
     array_append (new_ts->midi_notes,
                    new_ts->num_midi_notes,
                    new_r);
    }
  return new_ts;
}

void
midi_arranger_selections_paste_to_pos (
  MidiArrangerSelections * ts,
  Position *           pos)
{
  int pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  get_start_pos (ts,
                 &start_pos);
  int start_pos_ticks =
    position_to_ticks (&start_pos);

  /* subtract the start pos from every object and
   * add the given pos */
#define DIFF (curr_ticks - start_pos_ticks)
#define ADJUST_POSITION(x) \
  curr_ticks = position_to_ticks (x); \
  position_from_ticks (x, pos_ticks + DIFF)

  int curr_ticks, i;
  for (i = 0; i < ts->num_midi_notes; i++)
    {
      MidiNote * midi_note = ts->midi_notes[i];

      /* update positions */
      curr_ticks =
        position_to_ticks (&midi_note->start_pos);
      position_from_ticks (
        &midi_note->start_pos,
        pos_ticks + DIFF);
      curr_ticks =
        position_to_ticks (&midi_note->end_pos);
      position_from_ticks (&midi_note->end_pos,
                           pos_ticks + DIFF);

      /* clone and add to track */
      MidiNote * cp =
        midi_note_clone (midi_note,
                      MIDI_NOTE_CLONE_COPY);
      /*midi_note_print (cp);*/
      midi_region_add_midi_note (
        cp->midi_region,
        cp);
    }
#undef DIFF
}

void
midi_arranger_selections_free (
  MidiArrangerSelections * self)
{
  free (self);
}

SERIALIZE_SRC (MidiArrangerSelections,
               midi_arranger_selections)
DESERIALIZE_SRC (MidiArrangerSelections,
                 midi_arranger_selections)
PRINT_YAML_SRC (MidiArrangerSelections,
                midi_arranger_selections)

