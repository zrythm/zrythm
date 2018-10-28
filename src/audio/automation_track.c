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
#include "audio/track.h"
#include "gui/widgets/automation_track.h"

AutomationTrack *
automation_track_new (Track *         track,
                      Automatable *   a)
{
  AutomationTrack * at = calloc (1, sizeof (AutomationTrack));

  at->track = track;
  automation_track_set_automatable (at, a);

  /* create some test automation points TODO */
  for (int i = 0; i < 7; i++)
    {
      if (automatable_is_float (a))
        {
          AutomationPoint * ap = &at->automation_points[i];
          position_init (&ap->pos);
          ap->pos.bars = i + 2;
          ap->type = AUTOMATION_POINT_VALUE;
          float max = automatable_get_maxf (a);
          float min = automatable_get_minf (a);
          ap->fvalue = max - ((max - min) / 2);
          at->num_automation_points++;
          g_message ("created automation point with val %f",
                     ap->fvalue);
        }
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

/**
 * Updates automation track & its GUI
 */
void
automation_track_update (AutomationTrack * at)
{
  automation_track_widget_update (at->widget);
}

/**
 * Gets automation track for given automatable, if any.
 */
AutomationTrack *
automation_track_get_for_automatable (Automatable * automatable)
{
  Track * track = automatable->track;

  for (int i = 0; i < track->num_automation_tracks; i++)
    {
      AutomationTrack * at = track->automation_tracks[i];
      if (at->automatable == automatable)
        {
          return at;
        }
    }
  return NULL;
}

void
automation_track_free (AutomationTrack * at)
{
  /* FIXME free allocated members too */
  free (at);
}
