/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __UNDO_CREATE_TRACKS_ACTION_H__
#define __UNDO_CREATE_TRACKS_ACTION_H__

#include "actions/undoable_action.h"
#include "gui/backend/file_manager.h"
#include "plugins/plugin.h"
#include "audio/track.h"

/**
 * @addtogroup actions
 *
 * @{
 */


typedef struct CreateTracksAction
{
  UndoableAction  parent_instance;

  /** Position to make the tracks at. */
  int              pos;

  /** Number of tracks to make. */
  int              num_tracks;

  /** Track type. */
  TrackType        type;

  /** ID of the Tracks added (for easy undoing). */
  int              track_ids[60];

  /** Flag to know if we are making an empty track. */
  int              is_empty;

  /** PluginDescriptor, if making an instrument or
   * bus track from a plugin.
   *
   * If this is empty and the track type is instrument,
   * it is assumed that it's an empty track. */
  PluginDescriptor pl_descr;

  /** Filename, if we are making an audio track from
   * a file. */
  FileDescriptor   file_descr;
} CreateTracksAction;

UndoableAction *
create_tracks_action_new (
  TrackType          type,
  PluginDescriptor * pl_descr,
  FileDescriptor *   file_descr,
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
