/*
 * actions/timeline_selections.c - timeline
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/timeline_selections.h"
#include "project.h"
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
      curr_ticks = position_to_ticks (&region->unit_end_pos);
      position_from_ticks (&region->unit_end_pos,
                           pos_ticks + DIFF);

      /* clone and add to track */
      Region * cp =
        region_clone (region,
                      REGION_CLONE_COPY);
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

X_SERIALIZE_SRC (TimelineSelections, timeline_selections)
X_DESERIALIZE_SRC (TimelineSelections, timeline_selections)
