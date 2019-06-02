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

#include "audio/automation_lane.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"

void
automation_lane_init_loaded (
  AutomationLane * self)
{
  self->widget =
    automation_lane_widget_new (self);
}

/**
 * Creates an automation lane for the given
 * automation track.
 */
AutomationLane *
automation_lane_new (
  AutomationTrack * at)
{
  AutomationLane * self =
    calloc (1, sizeof (AutomationLane));

  self->at = at;
  at->al = self;

  /* visible by default */
  self->visible = 1;

  return self;
}

/**
 * Updates the automation track in this lane and
 * updates the UI to reflect the change.
 *
 * TODO
 */
void
automation_lane_update_automation_track (
  AutomationLane * self,
  AutomationTrack * at)
{
  /* remove automation lane from previous at */
  self->at->al = NULL;

  /* set automation lane to newly selected at */
  at->al = self;

  /* set newly selected at to automation lane */
  self->at = at;

  automation_tracklist_update (
    track_get_automation_tracklist (at->track));

  EVENTS_PUSH (
    ET_AUTOMATION_LANE_AUTOMATION_TRACK_CHANGED,
    self);
}

void
automation_lane_free (AutomationLane * self)
{
  if (self->widget && GTK_IS_WIDGET (self->widget))
    gtk_widget_destroy (GTK_WIDGET (self->widget));

  free (self);
}
