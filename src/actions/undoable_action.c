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

#include "actions/create_midi_arranger_selections_action.h"
#include "actions/create_timeline_selections_action.h"
#include "actions/delete_midi_arranger_selections_action.h"
#include "actions/delete_timeline_selections_action.h"
#include "actions/duplicate_midi_arranger_selections_action.h"
#include "actions/duplicate_timeline_selections_action.h"
#include "actions/edit_channel_action.h"
#include "actions/edit_track_action.h"
#include "actions/move_midi_arranger_selections_action.h"
#include "actions/move_timeline_selections_action.h"
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
      edit_channel_action_do ((EditChannelAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_EDIT_TRACK:
      edit_track_action_do ((EditTrackAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_TL_SELECTIONS:
      create_timeline_selections_action_do (
        (CreateTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_TL_SELECTIONS:
      delete_timeline_selections_action_do (
        (DeleteTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_TL_SELECTIONS:
      move_timeline_selections_action_do (
        (MoveTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DUPLICATE_TL_SELECTIONS:
      duplicate_timeline_selections_action_do (
        (DuplicateTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_MA_SELECTIONS:
      create_midi_arranger_selections_action_do (
        (CreateMidiArrangerSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DUPLICATE_MA_SELECTIONS:
      duplicate_midi_arranger_selections_action_do (
        (DuplicateMidiArrangerSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_MA_SELECTIONS:
      delete_midi_arranger_selections_action_do (
        (DeleteMidiArrangerSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_MA_SELECTIONS:
      move_midi_arranger_selections_action_do(
        (MoveMidiArrangerSelectionsAction *) self);
      break;
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
      edit_channel_action_undo ((EditChannelAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_EDIT_TRACK:
      edit_track_action_undo ((EditTrackAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_TL_SELECTIONS:
      create_timeline_selections_action_undo (
        (CreateTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_TL_SELECTIONS:
      delete_timeline_selections_action_undo (
        (DeleteTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_TL_SELECTIONS:
      move_timeline_selections_action_undo (
        (MoveTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DUPLICATE_TL_SELECTIONS:
      duplicate_timeline_selections_action_undo (
        (DuplicateTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_MA_SELECTIONS:
      create_midi_arranger_selections_action_undo (
        (CreateMidiArrangerSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DUPLICATE_MA_SELECTIONS:
      duplicate_midi_arranger_selections_action_undo (
        (DuplicateMidiArrangerSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_MA_SELECTIONS:
     move_midi_arranger_selections_action_undo (
       (MoveMidiArrangerSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_MA_SELECTIONS:
      delete_midi_arranger_selections_action_undo (
        (DeleteMidiArrangerSelectionsAction *) self);
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
      edit_channel_action_free ((EditChannelAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_CHANNEL:
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_TL_SELECTIONS:
      create_timeline_selections_action_free (
        (CreateTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_TL_SELECTIONS:
      delete_timeline_selections_action_free (
        (DeleteTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_TL_SELECTIONS:
      move_timeline_selections_action_free (
        (MoveTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DUPLICATE_TL_SELECTIONS:
      duplicate_timeline_selections_action_free (
        (DuplicateTimelineSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_CREATE_MA_SELECTIONS:
      create_midi_arranger_selections_action_free (
        (CreateMidiArrangerSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DUPLICATE_MA_SELECTIONS:
      duplicate_midi_arranger_selections_action_free (
        (DuplicateMidiArrangerSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_MOVE_MA_SELECTIONS:
      move_midi_arranger_selections_action_free (
        (MoveMidiArrangerSelectionsAction *) self);
      break;
    case UNDOABLE_ACTION_TYPE_DELETE_MA_SELECTIONS:
      delete_midi_arranger_selections_action_free (
        (DeleteMidiArrangerSelectionsAction *) self);
      break;
    }
}
