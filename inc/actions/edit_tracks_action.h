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

/**
 * \file
 *
 * Edit tracks action.
 */

#ifndef __UNDO_EDIT_TRACKS_ACTION_H__
#define __UNDO_EDIT_TRACKS_ACTION_H__

#include "actions/undoable_action.h"
#include "gui/backend/tracklist_selections.h"

typedef struct Track Track;
typedef struct TracklistSelections
  TracklistSelections;

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Action type.
 */
typedef enum EditTracksActionType
{
  EDIT_TRACK_ACTION_TYPE_SOLO,
  EDIT_TRACK_ACTION_TYPE_MUTE,
  EDIT_TRACK_ACTION_TYPE_VOLUME,
  EDIT_TRACK_ACTION_TYPE_PAN,

  /** Direct out change. */
  EDIT_TRACK_ACTION_TYPE_DIRECT_OUT,

  /** Rename track. */
  EDIT_TRACK_ACTION_TYPE_RENAME,

  EDIT_TRACK_ACTION_TYPE_COLOR,
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
  { "Rename",
    EDIT_TRACK_ACTION_TYPE_RENAME    },
  { "Color",
    EDIT_TRACK_ACTION_TYPE_COLOR    },
};

/**
 * Edit tracks action.
 */
typedef struct EditTracksAction
{
  UndoableAction        parent_instance;
  EditTracksActionType  type;

  /** Tracklist selections before the change */
  TracklistSelections * tls_before;

  /** Tracklist selections after the change */
  TracklistSelections * tls_after;

  /* TODO remove the following and use
   * before/after */

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

  GdkRGBA               new_color;

  /** Skip do if true. */
  bool                  already_edited;

} EditTracksAction;

static const cyaml_schema_field_t
  edit_tracks_action_fields_schema[] =
{
  YAML_FIELD_MAPPING_EMBEDDED (
    EditTracksAction, parent_instance,
    undoable_action_fields_schema),
  YAML_FIELD_ENUM (
    EditTracksAction, type,
    edit_tracks_action_type_strings),
  YAML_FIELD_MAPPING_PTR (
    EditTracksAction, tls_before,
    tracklist_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    EditTracksAction, tls_after,
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
  YAML_FIELD_MAPPING_EMBEDDED (
    EditTracksAction, new_color,
    gdk_rgba_fields_schema),

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
 * Generic edit action.
 */
UndoableAction *
edit_tracks_action_new_generic (
  EditTracksActionType  type,
  TracklistSelections * tls_before,
  TracklistSelections * tls_after,
  bool                  already_edited);

/**
 * Generic edit action.
 */
UndoableAction *
edit_tracks_action_new_track_float (
  EditTracksActionType  type,
  Track *               track,
  float                 val_before,
  float                 val_after,
  bool                  already_edited);

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

/**
 * Wrapper over edit_tracks_action_new().
 */
UndoableAction *
edit_tracks_action_new_color (
  TracklistSelections * tls,
  GdkRGBA *             color);

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

/**
 * @}
 */

#endif
