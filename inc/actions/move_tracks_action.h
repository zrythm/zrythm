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

#ifndef __UNDO_MOVE_TRACKS_ACTION_H__
#define __UNDO_MOVE_TRACKS_ACTION_H__

#include "actions/undoable_action.h"
#include "audio/track.h"
#include "gui/backend/tracklist_selections.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef struct TracklistSelections TracklistSelections;

typedef struct MoveTracksAction
{
  UndoableAction        parent_instance;

  /** Position to move the tracks to. */
  int                   pos;

  TracklistSelections * tls;
} MoveTracksAction;

static const cyaml_schema_field_t
  move_tracks_action_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "parent_instance", CYAML_FLAG_DEFAULT,
    MoveTracksAction, parent_instance,
    undoable_action_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "tls", CYAML_FLAG_POINTER,
    MoveTracksAction, tls,
    tracklist_selections_fields_schema),
  CYAML_FIELD_INT (
    "pos", CYAML_FLAG_DEFAULT,
    MoveTracksAction, pos),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  move_tracks_action_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, MoveTracksAction,
    move_tracks_action_fields_schema),
};

void
move_tracks_action_init_loaded (
  MoveTracksAction * self);

/**
 * Move tracks to given position.
 */
UndoableAction *
move_tracks_action_new (
  TracklistSelections * tls,
  int      pos);

int
move_tracks_action_do (
  MoveTracksAction * self);

int
move_tracks_action_undo (
  MoveTracksAction * self);

char *
move_tracks_action_stringize (
  MoveTracksAction * self);

void
move_tracks_action_free (
  MoveTracksAction * self);

/**
 * @}
 */

#endif
