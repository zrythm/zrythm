/*
 * audio/automation_point.c - Automation point
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

/** \file
 */

#include "audio/automatable.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/position.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"

/**
 * Creates automation point in given track at given Position
 */
AutomationPoint *
automation_point_new_float (AutomationTrack *   at,
                            AutomationPointType type,
                            float               value,
                            Position *          pos)
{
  AutomationPoint * ap = calloc (1, sizeof (AutomationPoint));

  ap->at = at;
  position_set_to_pos (&ap->pos,
                       pos);
  ap->fvalue = value;
  ap->widget = automation_point_widget_new (ap);

  return ap;
}

/**
 * Returns Y in pixels from the value based on the allocation of the
 * automation track.
 */
int
automation_point_get_y_in_px (AutomationPoint * ap)
{
  Automatable * a = ap->at->automatable;
  float ap_max = automatable_get_maxf (a);
  float ap_range = ap_max - automatable_get_minf (a);
  float ap_ratio = (ap_range - (ap_max - ap->fvalue)) / ap_range;
  int allocated_h =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (ap->at->widget->at_grid));
  int point = allocated_h - ap_ratio * allocated_h;
  return point;
}
