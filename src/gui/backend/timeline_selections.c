/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/backend/timeline_selections.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

/**
 * Returns the position of the leftmost object.
 */
static void
get_start_pos (
  TimelineSelections * ts,
  Position *           pos) ///< position to fill in
{
  position_set_to_bar (pos,
                       TRANSPORT->total_bars);

  for (int i = 0; i < ts->num_regions; i++)
    {
      Region * region = ts->regions[i];
      if (position_compare (&region->start_pos,
                            pos) < 0)
        position_set_to_pos (pos,
                             &region->start_pos);
    }
  for (int i = 0; i < ts->num_automation_points; i++)
    {
      AutomationPoint * automation_point =
        ts->automation_points[i];
      if (position_compare (&automation_point->pos,
                            pos) < 0)
        position_set_to_pos (pos,
                             &automation_point->pos);
    }
  for (int i = 0; i < ts->num_chords; i++)
    {
      Chord * chord = ts->chords[i];
      if (position_compare (&chord->pos,
                            pos) < 0)
        position_set_to_pos (pos,
                             &chord->pos);
    }
}

/**
 * Clone the struct for copying, undoing, etc.
 */
TimelineSelections *
timeline_selections_clone ()
{
  TimelineSelections * new_ts =
    calloc (1, sizeof (TimelineSelections));

  TimelineSelections * src = TIMELINE_SELECTIONS;

  for (int i = 0; i < src->num_regions; i++)
    {
      Region * r = src->regions[i];
      Region * new_r =
        region_clone (r, REGION_CLONE_COPY);
      array_append (new_ts->regions,
                    new_ts->num_regions,
                    new_r);
    }

  return new_ts;
}

void
timeline_selections_paste_to_pos (
  TimelineSelections * ts,
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

  g_message ("[before loop]num regions %d num midi notes %d",
             ts->num_regions,
             ts->regions[0]->num_midi_notes);

  int curr_ticks, i;
  for (i = 0; i < ts->num_regions; i++)
    {
      Region * region = ts->regions[i];

      /* update positions */
      curr_ticks = position_to_ticks (&region->start_pos);
      position_from_ticks (&region->start_pos,
                           pos_ticks + DIFF);
      curr_ticks = position_to_ticks (&region->end_pos);
      position_from_ticks (&region->end_pos,
                           pos_ticks + DIFF);
      /* TODO */
      /*position_set_to_pos (&region->unit_end_pos,*/
                           /*&region->end_pos);*/
  g_message ("[in loop]num regions %d num midi notes %d",
             ts->num_regions,
             ts->regions[0]->num_midi_notes);

      /* same for midi notes */
      g_message ("region type %d", region->type);
      if (region->type == REGION_TYPE_MIDI)
        {
          MidiRegion * mr = region;
          g_message ("HELLO?");
          g_message ("num midi notes here %d",
                     mr->num_midi_notes);
          for (int j = 0; j < mr->num_midi_notes; j++)
            {
              MidiNote * mn = mr->midi_notes[j];
              g_message ("old midi start");
              /*position_print (&mn->start_pos);*/
              g_message ("bars %d",
                         mn->start_pos.bars);
              g_message ("new midi start");
              ADJUST_POSITION (&mn->start_pos);
              position_print (&mn->start_pos);
              g_message ("old midi start");
              ADJUST_POSITION (&mn->end_pos);
              position_print (&mn->end_pos);
            }
        }

      /* clone and add to track */
      Region * cp =
        region_clone (region,
                      REGION_CLONE_COPY);
      region_print (cp);
      track_add_region (cp->track,
                        cp);
    }
  for (i = 0; i < ts->num_automation_points; i++)
    {
      AutomationPoint * ap =
        ts->automation_points[i];

      curr_ticks = position_to_ticks (&ap->pos);
      position_from_ticks (&ap->pos,
                           pos_ticks + DIFF);
    }
  for (i = 0; i < ts->num_chords; i++)
    {
      Chord * chord = ts->chords[i];

      curr_ticks = position_to_ticks (&chord->pos);
      position_from_ticks (&chord->pos,
                           pos_ticks + DIFF);
    }
#undef DIFF
}

void
timeline_selections_free (TimelineSelections * self)
{
  free (self);
}

SERIALIZE_SRC (TimelineSelections, timeline_selections)
DESERIALIZE_SRC (TimelineSelections, timeline_selections)
PRINT_YAML_SRC (TimelineSelections, timeline_selections)
