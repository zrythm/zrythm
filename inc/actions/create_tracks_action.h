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

#ifndef __UNDO_CREATE_TRACKS_ACTION_H__
#define __UNDO_CREATE_TRACKS_ACTION_H__

#include "actions/undoable_action.h"
#include "plugins/plugin.h"
#include "audio/track.h"

typedef struct SupportedFile SupportedFile;

/**
 * @addtogroup actions
 *
 * @{
 */

typedef struct CreateTracksAction
{
  UndoableAction  parent_instance;

  /** Position to make the tracks at.
   *
   * Used when undoing too. */
  int              pos;

  /** Number of tracks to make. */
  int              num_tracks;

  /** Track type. */
  TrackType        type;

  /** Flag to know if we are making an empty
   * track. */
  int              is_empty;

  /** PluginDescriptor, if making an instrument or
   * bus track from a plugin.
   *
   * If this is empty and the track type is
   * instrument, it is assumed that it's an empty
   * track. */
  PluginDescriptor pl_descr;

  /** Filename, if we are making an audio track from
   * a file. */
  SupportedFile *  file_descr;

  /**
   * When this action is created, it will create
   * the audio file and add it to the pool.
   *
   * New audio regions should use this ID to avoid
   * duplication.
   */
  int              pool_id;
} CreateTracksAction;

static const cyaml_schema_field_t
  create_tracks_action_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "parent_instance", CYAML_FLAG_DEFAULT,
    CreateTracksAction, parent_instance,
    undoable_action_fields_schema),
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    CreateTracksAction, type, track_type_strings,
    CYAML_ARRAY_LEN (track_type_strings)),
  CYAML_FIELD_MAPPING (
    "pl_descr", CYAML_FLAG_DEFAULT,
    CreateTracksAction, pl_descr,
    plugin_descriptor_fields_schema),
  CYAML_FIELD_INT (
    "is_empty", CYAML_FLAG_DEFAULT,
    CreateTracksAction, is_empty),
  CYAML_FIELD_INT (
    "pos", CYAML_FLAG_DEFAULT,
    CreateTracksAction, pos),
  CYAML_FIELD_INT (
    "num_tracks", CYAML_FLAG_DEFAULT,
    CreateTracksAction, num_tracks),
  CYAML_FIELD_INT (
    "pool_id", CYAML_FLAG_DEFAULT,
    CreateTracksAction, pool_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  create_tracks_action_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, CreateTracksAction,
    create_tracks_action_fields_schema),
};

/**
 * Creates a new CreateTracksAction.
 *
 * @param pos Position to make the tracks at.
 */
UndoableAction *
create_tracks_action_new (
  TrackType          type,
  const PluginDescriptor * pl_descr,
  SupportedFile *    file_descr,
  int                pos,
  int                num_tracks);

int
create_tracks_action_do (
	CreateTracksAction * self);

int
create_tracks_action_undo (
	CreateTracksAction * self);

char *
create_tracks_action_stringize (
	CreateTracksAction * self);

void
create_tracks_action_free (
	CreateTracksAction * self);

/**
 * @}
 */

#endif
