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

/**
 * \file
 *
 * Transport action.
 */

#ifndef __ACTIONS_TRANSPORT_ACTION_H__
#define __ACTIONS_TRANSPORT_ACTION_H__

#include <stdbool.h>

#include "actions/undoable_action.h"
#include "audio/transport.h"
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
  TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE,
  TRANSPORT_ACTION_BEAT_UNIT_CHANGE,
} TransportActionType;

static const cyaml_strval_t
  transport_action_type_strings[] = {
    {"BPM change",            TRANSPORT_ACTION_BPM_CHANGE},
    { "beats per bar change",
     TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE               },
    { "beat unit change",
     TRANSPORT_ACTION_BEAT_UNIT_CHANGE                   },
};

/**
 * Transport action.
 */
typedef struct TransportAction
{
  UndoableAction parent_instance;

  TransportActionType type;

  bpm_t bpm_before;
  bpm_t bpm_after;

  int int_before;
  int int_after;

  /** Flag whether the action was already performed
   * the first time. */
  bool already_done;

  /** Whether musical mode was enabled when this
   * action was made. */
  bool musical_mode;
} TransportAction;

static const cyaml_schema_field_t
  transport_action_fields_schema[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      TransportAction,
      parent_instance,
      undoable_action_fields_schema),
    YAML_FIELD_ENUM (
      TransportAction,
      type,
      transport_action_type_strings),
    YAML_FIELD_FLOAT (TransportAction, bpm_before),
    YAML_FIELD_FLOAT (TransportAction, bpm_after),
    YAML_FIELD_INT (TransportAction, int_before),
    YAML_FIELD_INT (TransportAction, int_after),
    YAML_FIELD_INT (TransportAction, musical_mode),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  transport_action_schema = {
    YAML_VALUE_PTR (
      TransportAction,
      transport_action_fields_schema),
  };

void
transport_action_init_loaded (
  TransportAction * self);

WARN_UNUSED_RESULT
UndoableAction *
transport_action_new_bpm_change (
  bpm_t     bpm_before,
  bpm_t     bpm_after,
  bool      already_done,
  GError ** error);

WARN_UNUSED_RESULT
UndoableAction *
transport_action_new_time_sig_change (
  TransportActionType type,
  int                 before,
  int                 after,
  bool                already_done,
  GError **           error);

NONNULL
TransportAction *
transport_action_clone (const TransportAction * src);

bool
transport_action_perform_bpm_change (
  bpm_t     bpm_before,
  bpm_t     bpm_after,
  bool      already_done,
  GError ** error);

bool
transport_action_perform_time_sig_change (
  TransportActionType type,
  int                 before,
  int                 after,
  bool                already_done,
  GError **           error);

int
transport_action_do (
  TransportAction * self,
  GError **         error);

int
transport_action_undo (
  TransportAction * self,
  GError **         error);

char *
transport_action_stringize (TransportAction * self);

void
transport_action_free (TransportAction * self);

/**
 * @}
 */

#endif
