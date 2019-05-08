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

#ifndef __UNDO_EDIT_TRACKS_ACTION_H__
#define __UNDO_EDIT_TRACKS_ACTION_H__

#include "actions/undoable_action.h"

typedef enum EditTracksActionType
{
  EDIT_TRACK_ACTION_TYPE_SOLO,
  EDIT_TRACK_ACTION_TYPE_MUTE,
  EDIT_TRACK_ACTION_TYPE_VOLUME,
  EDIT_TRACK_ACTION_TYPE_PAN,
} EditTracksActionType;

typedef struct Track Track;
typedef struct TracklistSelections
  TracklistSelections;

typedef struct EditTracksAction
{
  UndoableAction        parent_instance;
  EditTracksActionType  type;

  TracklistSelections * tls;

  /**
   * The track that originated the action.
   */
  int                   main_track_pos;

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

} EditTracksAction;

/**
 * All-in-one constructor.
 *
 * Only the necessary params should be passed, others
 * will get ignored.
 */
UndoableAction *
edit_tracks_action_new (
  EditTracksActionType type,
  Track *              main_track,
  TracklistSelections * tls,
  float                vol_delta,
  float                pan_delta,
  int                  solo_new,
  int                  mute_new);

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
