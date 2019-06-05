/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file
 */

#include <math.h>

#include "audio/automatable.h"
#include "audio/automation_lane.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/port.h"
#include "audio/position.h"
#include "audio/track.h"
#include "gui/widgets/automation_lane.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_curve.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "project.h"

static AutomationPoint *
_create_new (
  AutomationTrack * at,
  const Position *        pos)
{
  AutomationPoint * ap =
    calloc (1, sizeof (AutomationPoint));

  ap->at = at;
  position_set_to_pos (
    &ap->pos, pos);

  return ap;
}

void
automation_point_init_loaded (
  AutomationPoint * ap)
{
  /* TODO */
  /*ap->at =*/
    /*project_get_automation_track (ap->at_id);*/

  ap->widget =
    automation_point_widget_new (ap);
}

/**
 * Finds the automation point in the project matching
 * the params of the given one.
 */
AutomationPoint *
automation_point_find (
  AutomationPoint * src)
{
  Track * track =
    TRACKLIST->tracks[src->track_pos];
  g_warn_if_fail (track);

  return track->automation_tracklist.
    ats[src->at_index]->aps[src->index];
}

/**
 * Creates an AutomationPoint in the given
 * AutomationTrack at the given Position.
 */
AutomationPoint *
automation_point_new_float (
  AutomationTrack *   at,
  const float         value,
  const Position *    pos)
{
  AutomationPoint * ap = _create_new (at, pos);

  ap->fvalue = value;
  ap->widget = automation_point_widget_new (ap);

  return ap;
}

/**
 * Returns Y in pixels from the value based on the
 * allocation of the automation track.
 */
int
automation_point_get_y_in_px (
  AutomationPoint * self)
{
  /* ratio of current value in the range */
  float ap_ratio =
    automation_point_get_normalized_value (self);

  int allocated_h =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self->at->al->widget));
  int point = allocated_h - ap_ratio * allocated_h;
  return point;
}

/**
 * Returns the normalized value (0.0 to 1.0).
 */
float
automation_point_get_normalized_value (
  AutomationPoint * self)
{
  /* TODO convert to macro */
  return automatable_real_val_to_normalized (
    self->at->automatable,
    self->fvalue);
}

/**
 * Returns the Track this AutomationPoint is in.
 */
Track *
automation_point_get_track (
  AutomationPoint * ap)
{
  return TRACKLIST->tracks[ap->track_pos];
}

/**
 * Updates the value from given real value and
 * notifies interested parties.
 */
void
automation_point_update_fvalue (
  AutomationPoint * self,
  float             real_val)
{
  self->fvalue = real_val;

  Automatable * a = self->at->automatable;
  automatable_set_val_from_normalized (
    a,
    automatable_real_val_to_normalized (a,
                                        real_val));
  AutomationCurve * ac =
    self->at->acs[self->index];
  if (ac && ac->widget)
    ac->widget->cache = 0;
}

/**
 * Destroys the widget and frees memory.
 */
void
automation_point_free (AutomationPoint * ap)
{
  gtk_widget_destroy (GTK_WIDGET (ap->widget));
  free (ap);
}
