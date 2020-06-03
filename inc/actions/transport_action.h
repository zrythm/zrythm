/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Transport action.
 */

#ifndef __ACTIONS_TRANSPORT_ACTION_H__
#define __ACTIONS_TRANSPORT_ACTION_H__

#include <stdbool.h>

#include "actions/undoable_action.h"
#include "utils/types.h"
#include "utils/yaml.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef enum TransportActionType
{
  TRANSPORT_ACTION_BPM_CHANGE,
} TransportActionType;

typedef struct TransportAction
{
  UndoableAction   parent_instance;

  bpm_t            bpm_before;
  bpm_t            bpm_after;

  /** Flag whether the action was already performed
   * the first time. */
  bool             already_done;

  /** Whether musical mode was enabled when this
   * action was made. */
  bool             musical_mode;
} TransportAction;

static const cyaml_schema_field_t
  transport_action_fields_schema[] =
{
  YAML_FIELD_MAPPING_EMBEDDED (
    TransportAction, parent_instance,
    undoable_action_fields_schema),
  YAML_FIELD_FLOAT (
    TransportAction, bpm_before),
  YAML_FIELD_FLOAT (
    TransportAction, bpm_after),
  YAML_FIELD_INT (
    TransportAction, musical_mode),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  transport_action_schema =
{
  YAML_VALUE_PTR (
    TransportAction,
    transport_action_fields_schema),
};

UndoableAction *
transport_action_new_bpm_change (
  bpm_t           bpm_before,
  bpm_t           bpm_after,
  bool            already_done);

int
transport_action_do (
  TransportAction * self);

int
transport_action_undo (
  TransportAction * self);

char *
transport_action_stringize (
  TransportAction * self);

void
transport_action_free (
  TransportAction * self);

/**
 * @}
 */

#endif
