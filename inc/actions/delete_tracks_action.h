/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __UNDO_DELETE_TRACKS_ACTION_H__
#define __UNDO_DELETE_TRACKS_ACTION_H__

#include "actions/undoable_action.h"
#include "plugins/plugin.h"
#include "audio/track.h"

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

  /** Clone of the TracklistSelections to delete. */
  TracklistSelections * tls;
} DeleteTracksAction;

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
