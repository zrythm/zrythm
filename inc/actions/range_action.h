/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __UNDO_RANGE_ACTION_H__
#define __UNDO_RANGE_ACTION_H__

#include "actions/undoable_action.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "gui/backend/timeline_selections.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef enum RangeActionType
{
  RANGE_ACTION_INSERT_SILENCE,
  RANGE_ACTION_REMOVE,
} RangeActionType;

static const cyaml_strval_t
  range_action_type_strings[] = {
    {"Insert silence", RANGE_ACTION_INSERT_SILENCE},
    { "Remove",        RANGE_ACTION_REMOVE        },
};

typedef struct RangeAction
{
  UndoableAction parent_instance;

  /** Range positions. */
  Position start_pos;
  Position end_pos;

  /** Action type. */
  RangeActionType type;

  /** Selections before the action, starting from
   * objects intersecting with the start position and
   * ending in infinity. */
  TimelineSelections * sel_before;

  /** Selections after the action. */
  TimelineSelections * sel_after;

  /** A copy of the transport at the start of the
   * action. */
  Transport * transport;

  /** Whether this is the first run. */
  bool first_run;

} RangeAction;

static const cyaml_schema_field_t
  range_action_fields_schema[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      RangeAction,
      parent_instance,
      undoable_action_fields_schema),
    YAML_FIELD_MAPPING_EMBEDDED (
      RangeAction,
      start_pos,
      position_fields_schema),
    YAML_FIELD_MAPPING_EMBEDDED (
      RangeAction,
      end_pos,
      position_fields_schema),
    YAML_FIELD_ENUM (
      RangeAction,
      type,
      range_action_type_strings),
    YAML_FIELD_MAPPING_PTR (
      RangeAction,
      sel_before,
      timeline_selections_fields_schema),
    YAML_FIELD_MAPPING_PTR (
      RangeAction,
      sel_after,
      timeline_selections_fields_schema),
    YAML_FIELD_INT (RangeAction, first_run),
    YAML_FIELD_MAPPING_PTR (
      RangeAction,
      transport,
      transport_fields_schema),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  range_action_schema = {
    CYAML_VALUE_MAPPING (
      CYAML_FLAG_POINTER,
      RangeAction,
      range_action_fields_schema),
  };

void
range_action_init_loaded (RangeAction * self);

/**
 * Creates a new action.
 *
 * @param start_pos Range start.
 * @param end_pos Range end.
 */
WARN_UNUSED_RESULT
UndoableAction *
range_action_new (
  RangeActionType type,
  Position *      start_pos,
  Position *      end_pos,
  GError **       error);

#define range_action_new_insert_silence( \
  start, end, error) \
  range_action_new ( \
    RANGE_ACTION_INSERT_SILENCE, start, end, \
    error)

#define range_action_new_remove(start, end, error) \
  range_action_new ( \
    RANGE_ACTION_REMOVE, start, end, error)

NONNULL
RangeAction *
range_action_clone (const RangeAction * src);

bool
range_action_perform (
  RangeActionType type,
  Position *      start_pos,
  Position *      end_pos,
  GError **       error);

#define range_action_perform_insert_silence( \
  start, end, error) \
  range_action_perform ( \
    RANGE_ACTION_INSERT_SILENCE, start, end, \
    error)

#define range_action_perform_remove( \
  start, end, error) \
  range_action_perform ( \
    RANGE_ACTION_REMOVE, start, end, error)

int
range_action_do (
  RangeAction * self,
  GError **     error);

int
range_action_undo (
  RangeAction * self,
  GError **     error);

char *
range_action_stringize (RangeAction * self);

void
range_action_free (RangeAction * self);

/**
 * @}
 */

#endif
