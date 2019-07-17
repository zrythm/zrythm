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

#include "actions/create_automation_selections_action.h"
#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "audio/marker.h"
#include "audio/marker_track.h"
#include "audio/scale_object.h"
#include "audio/track.h"
#include "gui/backend/automation_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/automation_arranger.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * The given AutomationSelections must already
 * contain the created selections in the transient
 * arrays.
 *
 * Note: chord addresses are to be copied.
 */
UndoableAction *
create_automation_selections_action_new (
  AutomationSelections * ts)
{
  CreateAutomationSelectionsAction * self =
    calloc (1, sizeof (
                 CreateAutomationSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_CREATE_AUTOMATION_SELECTIONS;

  self->ts = automation_selections_clone (ts);

  return ua;
}

int
create_automation_selections_action_do (
  CreateAutomationSelectionsAction * self)
{
  int i;

  AutomationPoint * ap;
	for (i = 0;
       i < self->ts->num_automation_points; i++)
    {
      /* check if the ap already exists. due to
       * how the arranger creates regions, the region
       * should already exist the first time so no
       * need to do anything. when redoing we will
       * need to create a clone instead */
      if (automation_point_find (
            self->ts->automation_points[i]))
        continue;

      /* clone the clone */
      ap =
        automation_point_clone (
          self->ts->automation_points[i],
          AUTOMATION_POINT_CLONE_COPY_MAIN);

      /* add it to track */
      automation_track_add_ap (
        ap->at, ap, F_GEN_WIDGET,
        F_GEN_CURVE_POINTS);

      /* remember new index */
      automation_point_set_automation_track_and_index (
        self->ts->automation_points[i],
        ap->at, ap->index);
    }

  EVENTS_PUSH (ET_AUTOMATION_SELECTIONS_CREATED,
               NULL);

  return 0;
}

int
create_automation_selections_action_undo (
  CreateAutomationSelectionsAction * self)
{
  int i;
  AutomationPoint * ap;
  for (i = 0;
       i < self->ts->num_automation_points; i++)
    {
      /* get the actual region */
      ap =
        automation_point_find (
          self->ts->automation_points[i]);
      g_warn_if_fail (ap);

      /* remove it */
      automation_track_remove_ap (
        ap->at, ap, F_FREE);
    }
  EVENTS_PUSH (ET_AUTOMATION_SELECTIONS_REMOVED,
               NULL);

  return 0;
}

char *
create_automation_selections_action_stringize (
  CreateAutomationSelectionsAction * self)
{
  return g_strdup (
    _("Create Automation Point(s)"));
}

void
create_automation_selections_action_free (
  CreateAutomationSelectionsAction * self)
{
  automation_selections_free (self->ts);

  free (self);
}
