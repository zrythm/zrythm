/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "utils/yaml.h"

#include <gtk/gtk.h>

MidiNote *
midi_arranger_selections_get_highest_note (
  MidiArrangerSelections * mas)
{
  MidiNote * top_mn = mas->midi_notes[0];
  MidiNote * tmp;
  for (int i = 0;
       i < mas->num_midi_notes;
       i++)
    {
        tmp = mas->midi_notes[i];
      if (tmp->val >
            top_mn->val)
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

  MidiNote * bot_mn =
    mas->midi_notes[0];
  MidiNote * tmp;
  for (int i = 0;
       i < mas->num_midi_notes;
       i++)
    {
      tmp = mas->midi_notes[i];
      if (tmp->val <
            bot_mn->val)
        {
          bot_mn = tmp;
        }
    }
  return bot_mn;
}

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region Region to paste to.
 */
int
midi_arranger_selections_can_be_pasted (
  MidiArrangerSelections * ts,
  Position *               pos,
  Region *                 r)
{
  if (!r || r->type != REGION_TYPE_MIDI)
    return 0;

  ArrangerObject * r_obj =
    (ArrangerObject *) r;
  if (r_obj->pos.frames + pos->frames < 0)
    return 0;

  return 1;
}

void
midi_arranger_selections_paste_to_pos (
  MidiArrangerSelections * ts,
  Position *               playhead)
{
  g_return_if_fail (
    CLIP_EDITOR->region &&
    CLIP_EDITOR->region->type == REGION_TYPE_MIDI);

  /* get region-local pos */
  Position pos;
  pos.frames =
    region_timeline_frames_to_local (
      CLIP_EDITOR->region, playhead->frames, 0);
  position_from_frames (&pos, pos.frames);
  long pos_ticks = position_to_ticks (&pos);

  /* get pos of earliest object */
  Position start_pos;
  arranger_selections_get_start_pos (
    (ArrangerSelections *) ts, &start_pos, 0);
  long start_pos_ticks =
    position_to_ticks (&start_pos);

  /* subtract the start pos from every object and
   * add the given pos */
#define DIFF (curr_ticks - start_pos_ticks)
#define ADJUST_POSITION(x) \
  curr_ticks = position_to_ticks (x); \
  position_from_ticks (x, pos_ticks + DIFF)

  long curr_ticks;
  int i;
  for (i = 0; i < ts->num_midi_notes; i++)
    {
      MidiNote * midi_note = ts->midi_notes[i];
      ArrangerObject * mn_obj =
        (ArrangerObject *) midi_note;
      midi_note->region = CLIP_EDITOR->region;
      midi_note->region_name =
        g_strdup (CLIP_EDITOR->region->name);

      /* update positions */
      curr_ticks =
        position_to_ticks (&mn_obj->pos);
      position_from_ticks (
        &mn_obj->pos, pos_ticks + DIFF);
      curr_ticks =
        position_to_ticks (&mn_obj->end_pos);
      position_from_ticks (
        &mn_obj->end_pos, pos_ticks + DIFF);

      /* clone and add to track */
      MidiNote * cp =
        (MidiNote *)
        arranger_object_clone (
          (ArrangerObject *) midi_note,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);
      midi_region_add_midi_note (
        cp->region, cp);
    }
#undef DIFF
}

SERIALIZE_SRC (MidiArrangerSelections,
               midi_arranger_selections)
DESERIALIZE_SRC (MidiArrangerSelections,
                 midi_arranger_selections)
PRINT_YAML_SRC (MidiArrangerSelections,
                midi_arranger_selections)

