/*
 * audio/automation_track.c - Automation track
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

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_point.h"
#include "gui/widgets/automation_track.h"

AutomationTrack *
automation_track_new (Track *         track,
                      Automatable *   a)
{
  AutomationTrack * at = calloc (1, sizeof (AutomationTrack));

  at->track = track;
  automation_track_set_automatable (at, a);
  at->widget = automation_track_widget_new (at);

  /* create some test automation points TODO */
  AutomationPoint * ap = &at->automation_points[0];
  position_init (&ap->pos);
  ap->type = AUTOMATION_POINT_VALUE;
  if (IS_AUTOMATABLE_LV2_CONTROL (a))
    {
    }

  return at;
}

/**
 * Sets the automatable to the automation track and updates the GUI
 */
void
automation_track_set_automatable (AutomationTrack * automation_track,
                                  Automatable *     a)
{
  automation_track->automatable = a;

  /* TODO update GUI */

}

