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

#ifndef __ACTION_DELETE_TRACKS_ACTION_H__
#define __ACTION_DELETE_TRACKS_ACTION_H__

#include "actions/undoable_action.h"
#include "gui/backend/tracklist_selections.h"
#include "audio/track.h"
#include "plugins/plugin.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef struct TracklistSelections
  TracklistSelections;

typedef struct DeleteTracksAction
{
  UndoableAction  parent_instance;

  /** Source sends that need to be deleted/
   * recreated on do/undo. */
  ChannelSend *   src_sends;
  int             num_src_sends;
  int             src_sends_size;

  /** Clone of the TracklistSelections to delete. */
  TracklistSelections * tls;
} DeleteTracksAction;

static const cyaml_schema_field_t
  delete_tracks_action_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "parent_instance", CYAML_FLAG_DEFAULT,
    DeleteTracksAction, parent_instance,
    undoable_action_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "tls", CYAML_FLAG_POINTER,
    DeleteTracksAction, tls,
    tracklist_selections_fields_schema),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (
    DeleteTracksAction, src_sends,
    channel_send_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  delete_tracks_action_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, DeleteTracksAction,
    delete_tracks_action_fields_schema),
};

void
delete_tracks_action_init_loaded (
  DeleteTracksAction * self);

UndoableAction *
delete_tracks_action_new (
  TracklistSelections * tls);

int
delete_tracks_action_do (
  DeleteTracksAction * self);

int
delete_tracks_action_undo (
  DeleteTracksAction * self);

char *
delete_tracks_action_stringize (
  DeleteTracksAction * self);

void
delete_tracks_action_free (
  DeleteTracksAction * self);

/**
 * @}
 */

#endif
