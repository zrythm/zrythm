/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __UNDO_EDIT_TRACKS_ACTION_H__
#define __UNDO_EDIT_TRACKS_ACTION_H__

#include "actions/undoable_action.h"
#include "gui/backend/tracklist_selections.h"

typedef enum EditTracksActionType
{
  EDIT_TRACK_ACTION_TYPE_SOLO,
  EDIT_TRACK_ACTION_TYPE_MUTE,
  EDIT_TRACK_ACTION_TYPE_VOLUME,
  EDIT_TRACK_ACTION_TYPE_PAN,

  /** Direct out change. */
  EDIT_TRACK_ACTION_TYPE_DIRECT_OUT,
} EditTracksActionType;

static const cyaml_strval_t
  edit_tracks_action_type_strings[] =
{
  { "solo",
    EDIT_TRACK_ACTION_TYPE_SOLO    },
  { "mute",
    EDIT_TRACK_ACTION_TYPE_MUTE    },
  { "volume",
    EDIT_TRACK_ACTION_TYPE_VOLUME    },
  { "pan",
    EDIT_TRACK_ACTION_TYPE_PAN    },
  { "direct out",
    EDIT_TRACK_ACTION_TYPE_DIRECT_OUT    },
};

typedef struct Track Track;
typedef struct TracklistSelections
  TracklistSelections;

typedef struct EditTracksAction
{
  UndoableAction        parent_instance;
  EditTracksActionType  type;

  TracklistSelections * tls;

  /* --------------- DELTAS ---------------- */

  /** Difference in volume in the main track. */
  float                 vol_delta;
  /** Difference in pan in the main track. */
  float                 pan_delta;
  /** New solo value 1 or 0. */
  int                   solo_new;
  /** New mute value 1 or 0. */
  int                   mute_new;

  /* -------------- end DELTAS ------------- */

  /** Track position to direct output to. */
  int                   new_direct_out_pos;

} EditTracksAction;

static const cyaml_schema_field_t
  edit_tracks_action_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "parent_instance", CYAML_FLAG_DEFAULT,
    EditTracksAction, parent_instance,
    undoable_action_fields_schema),
  YAML_FIELD_ENUM (
    EditTracksAction, type,
    edit_tracks_action_type_strings),
  CYAML_FIELD_MAPPING_PTR (
    "tls", CYAML_FLAG_POINTER,
    EditTracksAction, tls,
    tracklist_selections_fields_schema),
  YAML_FIELD_INT (
    EditTracksAction, solo_new),
  YAML_FIELD_INT (
    EditTracksAction, mute_new),
  YAML_FIELD_FLOAT (
    EditTracksAction, vol_delta),
  YAML_FIELD_FLOAT (
    EditTracksAction, pan_delta),
  YAML_FIELD_INT (
    EditTracksAction, new_direct_out_pos),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  edit_tracks_action_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, EditTracksAction,
    edit_tracks_action_fields_schema),
};

void
edit_tracks_action_init_loaded (
  EditTracksAction * self);

/**
 * All-in-one constructor.
 *
 * Only the necessary params should be passed, others
 * will get ignored.
 */
UndoableAction *
edit_tracks_action_new (
  EditTracksActionType  type,
  TracklistSelections * tls,
  Track *               direct_out,
  float                 vol_delta,
  float                 pan_delta,
  bool                  solo_new,
  bool                  mute_new);

/**
 * Wrapper over edit_tracks_action_new().
 */
UndoableAction *
edit_tracks_action_new_mute (
  TracklistSelections * tls,
  bool                  mute_new);

/**
 * Wrapper over edit_tracks_action_new().
 */
UndoableAction *
edit_tracks_action_new_solo (
  TracklistSelections * tls,
  bool                  solo_new);

/**
 * Wrapper over edit_tracks_action_new().
 */
UndoableAction *
edit_tracks_action_new_direct_out (
  TracklistSelections * tls,
  Track *               direct_out);

int
edit_tracks_action_do (
  EditTracksAction * self);

int
edit_tracks_action_undo (
  EditTracksAction * self);

char *
edit_tracks_action_stringize (
  EditTracksAction * self);

void
edit_tracks_action_free (
  EditTracksAction * self);

#endif
