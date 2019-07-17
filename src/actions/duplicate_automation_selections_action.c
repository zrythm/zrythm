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

#include "actions/duplicate_automation_selections_action.h"
#include "audio/track.h"
#include "gui/backend/automation_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/automation_arranger.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
duplicate_automation_selections_action_new (
  AutomationSelections * ts,
  long                 ticks,
  int                  delta)
{
  DuplicateAutomationSelectionsAction * self =
    calloc (1, sizeof (
                 DuplicateAutomationSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_DUPLICATE_AUTOMATION_SELECTIONS;

  self->ts = automation_selections_clone (ts);
  self->ticks = ticks;
  self->delta = delta;

  return ua;
}

#define DO_OBJECT( \
  caps,cc,sc,add_to_track_code,remember_code) \
  cc * sc; \
	for (i = 0; i < self->ts->num_##sc##s; i++) \
    { \
      /* clone the clone */ \
      sc = \
        sc##_clone ( \
          self->ts->sc##s[i], \
          caps##_CLONE_COPY_MAIN); \
      /* add to track */ \
      add_to_track_code; \
      /* shift */ \
      sc##_shift_by_ticks ( \
        sc, self->ticks, AO_UPDATE_ALL); \
      /* shift the clone too so we can find it
       * when undoing */ \
      sc##_shift_by_ticks ( \
        self->ts->sc##s[i], self->ticks, \
        AO_UPDATE_THIS); \
      /* select it */ \
      sc##_widget_select ( \
        sc->widget, F_SELECT); \
      remember_code; \
    }

#define UNDO_OBJECT(cc,sc,remove_code) \
  cc * sc; \
	for (i = 0; i < self->ts->num_##sc##s; i++) \
    { \
      /* find the actual object */ \
      sc = \
        sc##_find ( \
          self->ts->sc##s[i]); \
      /* unselect it */ \
      sc##_widget_select ( \
        sc->widget, F_NO_SELECT); \
      /* remove it */ \
      remove_code; \
      /* unshift the clone */ \
      sc##_shift_by_ticks ( \
        self->ts->sc##s[i], - self->ticks, \
        AO_UPDATE_ALL); \
    }

int
duplicate_automation_selections_action_do (
  DuplicateAutomationSelectionsAction * self)
{
  int i;

  /*DO_OBJECT (*/
    /*AUTOMATION_POINT, AutomationPoint,*/
    /*automation_point,*/
    /*[> add <]*/
    /*automation_track_add_automation_point (*/
      /*TRACKLIST,*/
      /*scale_object, F_GEN_WIDGET),);*/

  EVENTS_PUSH (ET_AUTOMATION_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
duplicate_automation_selections_action_undo (
  DuplicateAutomationSelectionsAction * self)
{
  int i;

  /* TODO */
  /*UNDO_OBJECT (*/
    /*AutomationPoint, automation_point,*/
    /*[> remove <]*/
      /*g_message ("removing");*/
      /*position_print_simple (&marker->pos);*/
    /*marker_track_remove_marker (*/
      /*P_MARKER_TRACK,*/
      /*marker, F_FREE));*/

  EVENTS_PUSH (ET_AUTOMATION_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
duplicate_automation_selections_action_stringize (
  DuplicateAutomationSelectionsAction * self)
{
  return g_strdup (
    _("Duplicate Automation"));
}

void
duplicate_automation_selections_action_free (
  DuplicateAutomationSelectionsAction * self)
{
  automation_selections_free (self->ts);

  free (self);
}
