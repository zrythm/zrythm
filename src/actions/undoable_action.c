/*
 * actions/undoable_action.c - UndoableAction
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#include "actions/create_chords_action.h"
#include "actions/delete_timeline_selections_action.h"
#include "actions/edit_channel_action.h"
#include "actions/edit_track_action.h"
#include "actions/undoable_action.h"

/**
 * Performs the action.
 *
 * Note: only to be called by undo manager.
 */
void
undoable_action_do (UndoableAction * self)
{
  switch (self->type)
    {
    case UNDOABLE_ACTION_TYPE_CREATE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_EDIT_CHANNEL:
      edit_channel_action_do (
        (EditChannelAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_EDIT_TRACK:
      edit_track_action_do (
        (EditTrackAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_REGIONS:
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_REGIONS:
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_TIMELINE_SELECTIONS:
      delete_timeline_selections_action_do (
        (DeleteTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_CHORDS:
      create_chords_action_do (
        (CreateChordsAction *) self);
      break;
    }
}

/**
 * Undoes the action.
 *
 * Note: only to be called by undo manager.
 */
void
undoable_action_undo (UndoableAction * self)
{
  switch (self->type)
    {
    case UNDOABLE_ACTION_TYPE_CREATE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_EDIT_CHANNEL:
      edit_channel_action_undo (
        (EditChannelAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_EDIT_TRACK:
      edit_track_action_undo (
        (EditTrackAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_REGIONS:
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_REGIONS:
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_TIMELINE_SELECTIONS:
      delete_timeline_selections_action_undo (
        (DeleteTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_CHORDS:
      create_chords_action_undo (
        (CreateChordsAction *) self);
      break;
    }
}

void
undoable_action_free (UndoableAction * self)
{
  switch (self->type)
    {
    case UNDOABLE_ACTION_TYPE_CREATE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_EDIT_CHANNEL:
      edit_channel_action_free (
        (EditChannelAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_REGIONS:
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_REGIONS:
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_TIMELINE_SELECTIONS:
      delete_timeline_selections_action_free (
        (DeleteTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_CHORDS:
      create_chords_action_free (
        (CreateChordsAction *) self);
      break;
    }
}
