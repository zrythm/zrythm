/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/midi_note.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

MidiNote *
midi_arranger_selections_get_highest_note (
  MidiArrangerSelections * mas)
{
  MidiNote * top_mn = mas->midi_notes[0];
  MidiNote * tmp;
  for (int i = 0; i < mas->num_midi_notes; i++)
    {
      tmp = mas->midi_notes[i];
      if (tmp->val > top_mn->val)
        {
          top_mn = tmp;
        }
    }
  return top_mn;
}

MidiNote *
midi_arranger_selections_get_lowest_note (
  MidiArrangerSelections * mas)
{

  MidiNote * bot_mn = mas->midi_notes[0];
  MidiNote * tmp;
  for (int i = 0; i < mas->num_midi_notes; i++)
    {
      tmp = mas->midi_notes[i];
      if (tmp->val < bot_mn->val)
        {
          bot_mn = tmp;
        }
    }
  return bot_mn;
}

/**
 * Sets the listen status of notes on and off based
 * on changes in the previous selections and the
 * current selections.
 */
void
midi_arranger_selections_unlisten_note_diff (
  MidiArrangerSelections * prev,
  MidiArrangerSelections * mas)
{
  for (int i = 0; i < prev->num_midi_notes; i++)
    {
      MidiNote * prev_mn = prev->midi_notes[i];
      int        found = 0;
      for (int j = 0; j < mas->num_midi_notes; j++)
        {
          MidiNote * mn = mas->midi_notes[j];
          if (midi_note_is_equal (prev_mn, mn))
            {
              found = 1;
              break;
            }
        }

      if (!found)
        {
          midi_note_listen (prev_mn, 0);
        }
    }
}

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region ZRegion to paste to.
 */
int
midi_arranger_selections_can_be_pasted (
  MidiArrangerSelections * ts,
  Position *               pos,
  ZRegion *                r)
{
  if (!r || r->id.type != REGION_TYPE_MIDI)
    return 0;

  ArrangerObject * r_obj = (ArrangerObject *) r;
  if (r_obj->pos.frames + pos->frames < 0)
    return 0;

  return 1;
}
