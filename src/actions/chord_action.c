// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/chord_action.h"
#include "dsp/router.h"
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
chord_action_init_loaded (ChordAction * self)
{
}

/**
 * Creates a new action.
 *
 * @param chord Chord descriptor, if single chord.
 * @param chords_before Chord descriptors, if
 *   changing all chords.
 */
UndoableAction *
chord_action_new (
  const ChordDescriptor ** chords_before,
  const ChordDescriptor ** chords_after,
  const ChordDescriptor *  chord,
  const int                chord_idx,
  GError **                error)
{
  ChordAction *    self = object_new (ChordAction);
  UndoableAction * ua = (UndoableAction *) self;
  undoable_action_init (ua, UA_CHORD);

  g_return_val_if_fail (!chord || !chords_before, NULL);

  if (chord)
    {
      self->type = CHORD_ACTION_SINGLE;

      self->chord_before = chord_descriptor_clone (
        CHORD_EDITOR->chords[chord_idx]);
      self->chord_after = chord_descriptor_clone (chord);
      self->chord_idx = chord_idx;
    }
  else
    {
      self->type = CHORD_ACTION_ALL;

      self->chords_before = object_new_n (
        CHORD_EDITOR_NUM_CHORDS, ChordDescriptor *);
      self->chords_after = object_new_n (
        CHORD_EDITOR_NUM_CHORDS, ChordDescriptor *);

      for (size_t i = 0; i < CHORD_EDITOR_NUM_CHORDS; i++)
        {
          self->chords_before[i] =
            chord_descriptor_clone (chords_before[i]);
          self->chords_after[i] =
            chord_descriptor_clone (chords_after[i]);
        }
    }

  return ua;
}

ChordAction *
chord_action_clone (const ChordAction * src)
{
  ChordAction * self = object_new (ChordAction);
  self->parent_instance = src->parent_instance;

  self->type = src->type;
  self->chord_idx = src->chord_idx;

  switch (self->type)
    {
    case CHORD_ACTION_ALL:
      self->chords_before = object_new_n (
        CHORD_EDITOR_NUM_CHORDS, ChordDescriptor *);
      self->chords_after = object_new_n (
        CHORD_EDITOR_NUM_CHORDS, ChordDescriptor *);
      for (int i = 0; i < CHORD_EDITOR_NUM_CHORDS; i++)
        {
          self->chords_before[i] =
            chord_descriptor_clone (src->chords_before[i]);
          self->chords_after[i] =
            chord_descriptor_clone (src->chords_after[i]);
        }
      break;
    case CHORD_ACTION_SINGLE:
      self->chord_before =
        chord_descriptor_clone (src->chord_before);
      self->chord_after =
        chord_descriptor_clone (src->chord_before);
      break;
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
  const ChordDescriptor *  chord,
  const int                chord_idx,
  GError **                error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    chord_action_new, error, chords_before, chords_after,
    chord, chord_idx, error);
}

static int
do_or_undo (ChordAction * self, bool _do, GError ** error)
{
  switch (self->type)
    {
    case CHORD_ACTION_ALL:
      chord_editor_apply_chords (
        CHORD_EDITOR,
        (const ChordDescriptor **) (_do ? self->chords_after
                                        : self->chords_before),
        false);
      EVENTS_PUSH (ET_CHORDS_UPDATED, NULL);
      break;
    case CHORD_ACTION_SINGLE:
      chord_editor_apply_single_chord (
        CHORD_EDITOR,
        (_do ? self->chord_after : self->chord_before),
        self->chord_idx, false);
      EVENTS_PUSH (
        ET_CHORD_KEY_CHANGED,
        CHORD_EDITOR->chords[self->chord_idx]);
      break;
    }

  return 0;
}

int
chord_action_do (ChordAction * self, GError ** error)
{
  return do_or_undo (self, true, error);
}

int
chord_action_undo (ChordAction * self, GError ** error)
{
  return do_or_undo (self, false, error);
}

char *
chord_action_stringize (ChordAction * self)
{
  return g_strdup (_ ("Change chords"));
}

void
chord_action_free (ChordAction * self)
{
  if (self->type == CHORD_ACTION_ALL)
    {
      for (int i = 0; i < CHORD_EDITOR_NUM_CHORDS; i++)
        {
          object_free_w_func_and_null (
            chord_descriptor_free, self->chords_before[i]);
          object_free_w_func_and_null (
            chord_descriptor_free, self->chords_after[i]);
        }
      object_zero_and_free (self->chords_before);
      object_zero_and_free (self->chords_after);
    }
  else if (self->type == CHORD_ACTION_SINGLE)
    {
      object_free_w_func_and_null (
        chord_descriptor_free, self->chord_before);
      object_free_w_func_and_null (
        chord_descriptor_free, self->chord_after);
    }

  object_zero_and_free (self);
}
