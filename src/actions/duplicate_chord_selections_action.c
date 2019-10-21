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

#include "actions/duplicate_chord_selections_action.h"
#include "audio/chord_object.h"
#include "audio/chord_region.h"
#include "audio/chord_track.h"
#include "audio/marker.h"
#include "audio/marker_track.h"
#include "audio/scale_object.h"
#include "audio/track.h"
#include "gui/backend/chord_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/chord_arranger.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
duplicate_chord_selections_action_new (
  ChordSelections * ts,
  long                 ticks,
  int                  delta)
{
  DuplicateChordSelectionsAction * self =
    calloc (1, sizeof (
                 DuplicateChordSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_DUPLICATE_CHORD_SELECTIONS;

  self->ts = chord_selections_clone (ts);
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
      sc##_select ( \
        sc, F_SELECT); \
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
      sc##_select ( \
        sc, F_NO_SELECT); \
      /* remove it */ \
      remove_code; \
      /* unshift the clone */ \
      sc##_shift_by_ticks ( \
        self->ts->sc##s[i], - self->ticks, \
        AO_UPDATE_ALL); \
    }

int
duplicate_chord_selections_action_do (
  DuplicateChordSelectionsAction * self)
{
  int i;

  /* clear selections */
  chord_selections_clear (
    CHORD_SELECTIONS);

  DO_OBJECT (
    CHORD_OBJECT, ChordObject, chord_object,
    /* add */
    chord_region_add_chord_object (
      chord_object->region,
      chord_object),);

  EVENTS_PUSH (ET_CHORD_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
duplicate_chord_selections_action_undo (
  DuplicateChordSelectionsAction * self)
{
  int i;

  UNDO_OBJECT (
    ChordObject, chord_object,
    /* remove */
    chord_region_remove_chord_object (
      chord_object->region,
      chord_object, F_FREE));

  EVENTS_PUSH (ET_CHORD_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
duplicate_chord_selections_action_stringize (
  DuplicateChordSelectionsAction * self)
{
  return g_strdup (
    _("Duplicate Chord(s)"));
}

void
duplicate_chord_selections_action_free (
  DuplicateChordSelectionsAction * self)
{
  chord_selections_free (self->ts);

  free (self);
}

