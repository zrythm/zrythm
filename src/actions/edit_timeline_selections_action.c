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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/edit_timeline_selections_action.h"
#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"

#include <glib/gi18n.h>

UndoableAction *
edit_timeline_selections_action_new (
  TimelineSelections * ts,
  EditTimelineSelectionsType type,
  const long       ticks,
  const char *     str,
  const Position * pos)
{
	EditTimelineSelectionsAction * self =
    calloc (1, sizeof (
    	EditTimelineSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_EDIT_TL_SELECTIONS;

  self->ts = timeline_selections_clone (ts);
  self->type = type;
  self->ticks = ticks;
  if (str)
    self->str = g_strdup (str);
  if (pos)
    position_set_to_pos (
      &self->pos, pos);
  self->first_call = 1;

  return ua;
}

#define PROCESS_POS_CHANGE( \
  caps,sc,pos_name_caps,pos_name) \
  case ETS_##caps##_##pos_name_caps: \
    sc##_set_##pos_name ( \
      sc, &self->pos, AO_UPDATE_ALL); \
    /* switch positions */ \
    position_set_to_pos ( \
      &self->pos, \
      &self->ts->sc##s[i]->pos_name); \
    position_set_to_pos ( \
      &self->ts->sc##s[i]->pos_name, \
      &sc->pos_name); \
    break

int
edit_timeline_selections_action_do (
	EditTimelineSelectionsAction * self)
{
  int i;
  Region * region;
  for (i = 0; i < self->ts->num_regions; i++)
    {
      /* get the actual region */
      region = region_find (self->ts->regions[i]);
      g_warn_if_fail (region);

      switch (self->type)
        {
        case ETS_RESIZE_L:
          if (!self->first_call)
            {
              /* resize */
              region_resize (
                region, 1, self->ticks,
                AO_UPDATE_ALL);
            }
          break;
        case ETS_RESIZE_R:
          if (!self->first_call)
            {
              /* resize */
              region_resize (
                region, 0, self->ticks,
                AO_UPDATE_ALL);
            }
          break;
        case ETS_REGION_NAME:
          region->name =
            g_strdup (self->str);
          break;
        PROCESS_POS_CHANGE (
          REGION, region, CLIP_START_POS,
          clip_start_pos);
        PROCESS_POS_CHANGE (
          REGION, region, LOOP_START_POS,
          loop_start_pos);
        PROCESS_POS_CHANGE (
          REGION, region, LOOP_END_POS,
          loop_end_pos);
        case ETS_SPLIT:
          {
            region_split (
              region, &self->pos, 0,
              &self->r1, &self->r2);
            self->r1 =
              region_clone (
                self->r1, REGION_CLONE_COPY);
            self->r2 =
              region_clone (
                self->r2, REGION_CLONE_COPY);
          }
          break;
        default:
          g_warn_if_reached ();
          break;
        }
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  self->first_call = 0;

  return 0;
}

int
edit_timeline_selections_action_undo (
	EditTimelineSelectionsAction * self)
{
  int i;
  Region * region, * r1, * r2;
  for (i = 0; i < self->ts->num_regions; i++)
    {
      /* get the actual region */
      if (self->type == ETS_SPLIT)
        {
          r1 = region_find (self->r1);
          r2 = region_find (self->r2);
        }
      else
        {
          region =
            region_find (self->ts->regions[i]);
          g_warn_if_fail (region);
        }

      switch (self->type)
        {
        case ETS_RESIZE_L:
          /* resize */
          region_resize (
            region, 1, - self->ticks, AO_UPDATE_ALL);
          break;
        case ETS_RESIZE_R:
          /* resize */
          region_resize (
            region, 0, - self->ticks, AO_UPDATE_ALL);
          break;
        case ETS_REGION_NAME:
          region->name =
            g_strdup (self->ts->regions[i]->name);
          break;
        PROCESS_POS_CHANGE (
          REGION, region, CLIP_START_POS,
          clip_start_pos);
        PROCESS_POS_CHANGE (
          REGION, region, LOOP_START_POS,
          loop_start_pos);
        PROCESS_POS_CHANGE (
          REGION, region, LOOP_END_POS,
          loop_end_pos);
        case ETS_SPLIT:
          region_unsplit (
            r1, r2, &region);
          region_set_name (
            region, self->ts->regions[i]->name);
          region_free (self->r1);
          region_free (self->r2);
          break;
        default:
          g_warn_if_reached ();
          break;
        }
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
edit_timeline_selections_action_stringize (
	EditTimelineSelectionsAction * self)
{
  switch (self->type)
    {
      case ETS_RESIZE_L:
      case ETS_RESIZE_R:
        return g_strdup (_("Resize Object(s)"));
      case ETS_REGION_NAME:
        return g_strdup (_("Change Name"));
      case ETS_REGION_CLIP_START_POS:
        return g_strdup (_("Change Clip Start Position"));
      case ETS_REGION_LOOP_START_POS:
        return g_strdup (_("Change Loop Start Position"));
      case ETS_REGION_LOOP_END_POS:
        return g_strdup (_("Change Loop End Position"));
      case ETS_SPLIT:
        return g_strdup (_("Cut Region(s)"));
      default:
        g_return_val_if_reached (
          g_strdup (""));
    }
}

void
edit_timeline_selections_action_free (
	EditTimelineSelectionsAction * self)
{
  timeline_selections_free (self->ts);

  free (self);
}
