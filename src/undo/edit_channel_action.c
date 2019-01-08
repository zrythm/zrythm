/*
 * undo/edit_channel_action.c - UndoableAction
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "audio/channel.h"
#include "gui/widgets/channel.h"
#include "undo/edit_channel_action.h"

static EditChannelAction *
_create ()
{
  EditChannelAction * self =
    calloc (1, sizeof (EditChannelAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UNDOABLE_ACTION_TYPE_EDIT_CHANNEL;

  return self;
}

UndoableAction *
edit_channel_action_new_solo (Channel * channel)
{
  EditChannelAction * self = _create ();

  UndoableAction * ua = (UndoableAction *) self;
  self->channel = channel;
  self->type = EDIT_CHANNEL_ACTION_TYPE_SOLO;

  return ua;
}

UndoableAction *
edit_channel_action_new_mute (Channel * channel)
{
  EditChannelAction * self = _create ();

  UndoableAction * ua = (UndoableAction *) self;
  self->channel = channel;
  self->type = EDIT_CHANNEL_ACTION_TYPE_MUTE;

  return ua;
}

UndoableAction *
edit_channel_action_new_vol (Channel * channel,
                             float vol_prev,
                             float vol_new)
{
  EditChannelAction * self = _create ();

  UndoableAction * ua = (UndoableAction *) self;
  self->channel = channel;
  self->type = EDIT_CHANNEL_ACTION_TYPE_CHANGE_VOLUME;
  self->vol_prev = vol_prev;
  self->vol_new = vol_new;

  return ua;
}

UndoableAction *
edit_channel_action_new_pan (Channel * channel,
                             float pan_prev,
                             float pan_new)
{
  EditChannelAction * self = _create ();

  UndoableAction * ua = (UndoableAction *) self;
  self->channel = channel;
  self->type = EDIT_CHANNEL_ACTION_TYPE_CHANGE_PAN;
  self->pan_prev = pan_prev;
  self->pan_new = pan_new;

  return ua;
}

void
edit_channel_action_do (EditChannelAction * self)
{
  switch (self->type)
    {
    case EDIT_CHANNEL_ACTION_TYPE_SOLO:
      self->channel->widget->undo_redo_action = 1;
      channel_toggle_solo (self->channel);
      break;
    case EDIT_CHANNEL_ACTION_TYPE_MUTE:
      channel_toggle_mute (self->channel);
      break;
    case EDIT_CHANNEL_ACTION_TYPE_CHANGE_VOLUME:
      channel_set_fader_amp (self->channel,
                             self->vol_new);
      break;
    case EDIT_CHANNEL_ACTION_TYPE_CHANGE_PAN:
      channel_set_pan (self->channel,
                       self->pan_new);
      break;
    }
}

void
edit_channel_action_undo (EditChannelAction * self)
{
  switch (self->type)
    {
    case EDIT_CHANNEL_ACTION_TYPE_SOLO:
      self->channel->widget->undo_redo_action = 1;
      channel_toggle_solo (self->channel);
      break;
    case EDIT_CHANNEL_ACTION_TYPE_MUTE:
      channel_toggle_mute (self->channel);
      break;
    case EDIT_CHANNEL_ACTION_TYPE_CHANGE_VOLUME:
      channel_set_fader_amp (self->channel,
                             self->vol_prev);
      break;
    case EDIT_CHANNEL_ACTION_TYPE_CHANGE_PAN:
      channel_set_pan (self->channel,
                       self->pan_prev);
      break;
    }
}
