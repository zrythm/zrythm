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

#include "actions/quantize_timeline_selections.h"
#include "audio/quantize_options.h"
#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"

#include <glib/gi18n.h>

/**
 * Creates a new action.
 */
UndoableAction *
quantize_timeline_selections_action_new (
  const TimelineSelections * ts,
  const QuantizeOptions *    opts)
{
	QuantizeTimelineSelectionsAction * self =
    calloc (1, sizeof (
    	QuantizeTimelineSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_QUANTIZE_TL_SELECTIONS;

  self->unquantized_ts =
    timeline_selections_clone (ts);
  self->quantized_ts =
    timeline_selections_clone (ts);
  self->opts = quantize_options_clone (opts);

  return ua;
}

int
quantize_timeline_selections_action_do (
	QuantizeTimelineSelectionsAction * self)
{
  int i, ticks;
  Region * region;
  /*ScaleObject * scale_object;*/
  /*Marker * marker;*/
  TimelineSelections * ts = self->unquantized_ts;
  for (i = 0; i < ts->num_regions; i++)
    {
      /* get the actual region */
      region = region_find (ts->regions[i]);
      g_warn_if_fail (region);

      /* quantize it */
      if (self->opts->adj_start)
        {
          ticks =
            quantize_options_quantize_position (
              self->opts,
              &region->start_pos);
          position_add_ticks (
            &region->end_pos,
            ticks);
        }
      if (self->opts->adj_end)
        {
          ticks =
            quantize_options_quantize_position (
              self->opts,
              &region->end_pos);
          /*position_add_ticks (*/
            /*&region->start_pos,*/
            /*ticks);*/
        }
      region_set_start_pos (
        region, &region->start_pos,
        AO_UPDATE_ALL);
      region_set_end_pos (
        region, &region->end_pos,
        AO_UPDATE_ALL);

      /* remember the quantized position */
      position_set_to_pos (
        &self->quantized_ts->regions[i]->start_pos,
        &region->start_pos);
      position_set_to_pos (
        &self->quantized_ts->regions[i]->end_pos,
        &region->end_pos);
    }
  for (i = 0; i < ts->num_scale_objects; i++)
    {
      /* get the actual scale object */

      /* quantize it */

      /* remember the quantized position */
      /*position_set_to_pos (*/
        /*&self->quantized_ts->scale_objects[i]->pos,*/
        /*&scale_object->pos);*/
    }
  for (i = 0; i < ts->num_markers; i++)
    {
      /* get the actual scale object */

      /* quantize it */

      /* remember the quantized position */
      /*position_set_to_pos (*/
        /*&self->quantized_ts->markers[i]->pos,*/
        /*&marker->pos);*/
    }

  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
quantize_timeline_selections_action_undo (
	QuantizeTimelineSelectionsAction * self)
{
  int i;
  Region * region;
  TimelineSelections * ts = self->quantized_ts;
  for (i = 0; i < ts->num_regions; i++)
    {
      /* get the actual region */
      region = region_find (ts->regions[i]);
      g_warn_if_fail (region);

      /* unquantize it */
      region_set_start_pos (
        region,
        &self->unquantized_ts->regions[i]->start_pos,
        AO_UPDATE_ALL);
      region_set_end_pos (
        region,
        &self->unquantized_ts->regions[i]->end_pos,
        AO_UPDATE_ALL);
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
quantize_timeline_selections_action_stringize (
	QuantizeTimelineSelectionsAction * self)
{
  return g_strdup (_("Quantize Object(s)"));
}

void
quantize_timeline_selections_action_free (
	QuantizeTimelineSelectionsAction * self)
{
  timeline_selections_free (self->quantized_ts);
  timeline_selections_free (self->unquantized_ts);
  quantize_options_free (self->opts);

  free (self);
}
