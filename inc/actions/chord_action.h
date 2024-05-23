// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_CHORD_ACTION_H__
#define __UNDO_CHORD_ACTION_H__

#include "actions/undoable_action.h"
#include "dsp/chord_descriptor.h"
#include "gui/backend/chord_editor.h"

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Type of chord action.
 */
enum class ChordActionType
{
  /**
   * Change single chord.
   */
  CHORD_ACTION_SINGLE,

  /** Change all chords. */
  CHORD_ACTION_ALL,
};

/**
 * Action for chord pad changes.
 */
typedef struct ChordAction
{
  UndoableAction parent_instance;

  ChordActionType type;

  ChordDescriptor * chord_before;
  ChordDescriptor * chord_after;
  int               chord_idx;

  /** Chords before the change. */
  ChordDescriptor ** chords_before;

  /** Chords after the change. */
  ChordDescriptor ** chords_after;

} ChordAction;

void
chord_action_init_loaded (ChordAction * self);

/**
 * Creates a new action.
 *
 * @param chord Chord descriptor, if single chord.
 * @param chords_before Chord descriptors, if
 *   changing all chords.
 */
WARN_UNUSED_RESULT UndoableAction *
chord_action_new (
  const ChordDescriptor ** chords_before,
  const ChordDescriptor ** chords_after,
  const ChordDescriptor *  chord,
  const int                chord_idx,
  GError **                error);

NONNULL ChordAction *
chord_action_clone (const ChordAction * src);

/**
 * Wrapper to create action and perform it.
 */
bool
chord_action_perform (
  const ChordDescriptor ** chords_before,
  const ChordDescriptor ** chords_after,
  const ChordDescriptor *  chord,
  const int                chord_idx,
  GError **                error);

int
chord_action_do (ChordAction * self, GError ** error);

int
chord_action_undo (ChordAction * self, GError ** error);

char *
chord_action_stringize (ChordAction * self);

void
chord_action_free (ChordAction * self);

/**
 * @}
 */

#endif
