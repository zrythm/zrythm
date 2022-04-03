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

#ifndef __UNDO_CHORD_ACTION_H__
#define __UNDO_CHORD_ACTION_H__

#include "actions/undoable_action.h"
#include "audio/chord_descriptor.h"
#include "gui/backend/chord_editor.h"

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Type of chord action.
 */
typedef enum ChordActionType
{
  /**
   * Change single chord.
   */
  CHORD_ACTION_SINGLE,

  /** Change all chords. */
  CHORD_ACTION_ALL,
} ChordActionType;

static const cyaml_strval_t
  chord_action_type_strings[] = {
    {"Single", CHORD_ACTION_SINGLE},
    { "All",   CHORD_ACTION_ALL   },
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

static const cyaml_schema_field_t
  chord_action_fields_schema[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      ChordAction,
      parent_instance,
      undoable_action_fields_schema),
    YAML_FIELD_ENUM (
      ChordAction,
      type,
      chord_action_type_strings),
    YAML_FIELD_MAPPING_PTR_OPTIONAL (
      ChordAction,
      chord_before,
      chord_descriptor_fields_schema),
    YAML_FIELD_MAPPING_PTR_OPTIONAL (
      ChordAction,
      chord_after,
      chord_descriptor_fields_schema),
    YAML_FIELD_INT (ChordAction, chord_idx),
    YAML_FIELD_DYN_FIXED_SIZE_PTR_ARRAY_OPT (
      ChordAction,
      chords_before,
      chord_descriptor_schema,
      CHORD_EDITOR_NUM_CHORDS),
    YAML_FIELD_DYN_FIXED_SIZE_PTR_ARRAY_OPT (
      ChordAction,
      chords_after,
      chord_descriptor_schema,
      CHORD_EDITOR_NUM_CHORDS),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t chord_action_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ChordAction,
    chord_action_fields_schema),
};

void
chord_action_init_loaded (ChordAction * self);

/**
 * Creates a new action.
 *
 * @param chord Chord descriptor, if single chord.
 * @param chords_before Chord descriptors, if
 *   changing all chords.
 */
WARN_UNUSED_RESULT
UndoableAction *
chord_action_new (
  const ChordDescriptor ** chords_before,
  const ChordDescriptor ** chords_after,
  const ChordDescriptor *  chord,
  const int                chord_idx,
  GError **                error);

NONNULL
ChordAction *
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
chord_action_undo (
  ChordAction * self,
  GError **     error);

char *
chord_action_stringize (ChordAction * self);

void
chord_action_free (ChordAction * self);

/**
 * @}
 */

#endif
