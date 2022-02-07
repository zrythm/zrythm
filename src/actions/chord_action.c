/*
 * Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/chord_action.h"
#include "audio/router.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
chord_action_init_loaded (
  ChordAction * self)
{
}

/**
 * Creates a new action.
 */
UndoableAction *
chord_action_new (
  const ChordDescriptor ** chords_before,
  const ChordDescriptor ** chords_after,
  GError **                error)
{
  ChordAction * self =
    object_new (ChordAction);
  UndoableAction * ua = (UndoableAction *) self;
  undoable_action_init (ua, UA_CHORD);

  for (size_t i = 0; i < CHORD_EDITOR_NUM_CHORDS; i++)
    {
      self->chords_before[i] =
        chord_descriptor_clone (chords_before[i]);
      self->chords_after[i] =
        chord_descriptor_clone (chords_after[i]);
    }

  return ua;
}

ChordAction *
chord_action_clone (
  const ChordAction * src)
{
  ChordAction * self = object_new (ChordAction);
  self->parent_instance = src->parent_instance;

  for (int i = 0; i < CHORD_EDITOR_NUM_CHORDS; i++)
    {
      self->chords_before[i] =
        chord_descriptor_clone (src->chords_before[i]);
      self->chords_after[i] =
        chord_descriptor_clone (src->chords_after[i]);
    }

  return self;
}

/**
 * Wrapper to create action and perform it.
 */
bool
chord_action_perform (
  const ChordDescriptor ** chords_before,
  const ChordDescriptor ** chords_after,
  GError **                error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    chord_action_new, error,
    chords_before, chords_after, error);
}

static int
do_or_undo (
  ChordAction * self,
  bool          _do,
  GError **     error)
{
  chord_editor_apply_chords (
    CHORD_EDITOR,
    (const ChordDescriptor **)
    (_do ? self->chords_after : self->chords_before),
    false);

  EVENTS_PUSH (ET_CHORDS_UPDATED, NULL);

  return 0;
}

int
chord_action_do (
  ChordAction * self,
  GError **     error)
{
  return do_or_undo (self, true, error);
}

int
chord_action_undo (
  ChordAction * self,
  GError **     error)
{
  return do_or_undo (self, false, error);
}

char *
chord_action_stringize (
  ChordAction * self)
{
  return g_strdup (_("Change chords"));
}

void
chord_action_free (
  ChordAction * self)
{
  for (int i = 0; i < CHORD_EDITOR_NUM_CHORDS; i++)
    {
      object_free_w_func_and_null (
        chord_descriptor_free, self->chords_before[i]);
      object_free_w_func_and_null (
        chord_descriptor_free, self->chords_after[i]);
    }

  object_zero_and_free (self);
}
