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

#include "actions/edit_track_action.h"
#include "audio/channel.h"
#include "audio/track.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/track.h"
#include "project.h"

static EditTrackAction *
_create ()
{
  EditTrackAction * self =
    calloc (1, sizeof (EditTrackAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UNDOABLE_ACTION_TYPE_EDIT_TRACK;

  return self;
}

UndoableAction *
edit_track_action_new_solo (Track * track,
                              int       solo)
{
  EditTrackAction * self = _create ();

  UndoableAction * ua = (UndoableAction *) self;
  self->track_id = track->id;
  self->type = EDIT_TRACK_ACTION_TYPE_SOLO;

  self->solo_new = solo;

  return ua;
}

UndoableAction *
edit_track_action_new_mute (Track * track,
                              int       mute)
{
  EditTrackAction * self = _create ();

  UndoableAction * ua = (UndoableAction *) self;
  self->track_id = track->id;
  self->type = EDIT_TRACK_ACTION_TYPE_MUTE;

  self->mute_new = mute;

  return ua;
}

static int
refresh_chan_buttons_async (Channel * chan)
{
  channel_widget_block_all_signal_handlers (
    chan->widget);
  channel_widget_refresh_buttons (chan->widget);
  channel_widget_unblock_all_signal_handlers (
    chan->widget);

  return FALSE;
}

static int
refresh_track_buttons_async (Track * track)
{
  track_widget_block_all_signal_handlers (
    track->widget);
  track_widget_refresh_buttons (track->widget);
  track_widget_unblock_all_signal_handlers (
    track->widget);

  return FALSE;
}

/**
 * Refreshes the channel and track widgets.
 */
static void
refresh_chan_and_track_widgets (
  EditTrackAction * self)
{
  Track * track = PROJECT->tracks[self->track_id];
  Channel * chan = track_get_channel (track);
  switch (self->type)
    {
    case EDIT_TRACK_ACTION_TYPE_SOLO:
    case EDIT_TRACK_ACTION_TYPE_MUTE:
    case EDIT_TRACK_ACTION_TYPE_RECORD:
      if (chan && chan->widget)
        {
          g_idle_add (
            (GSourceFunc) refresh_chan_buttons_async,
            chan);
        }
      if (track->widget)
        {
          g_idle_add (
            (GSourceFunc) refresh_track_buttons_async,
            track);
        }
      break;
    }
}

void
edit_track_action_do (EditTrackAction * self)
{
  Track * track = PROJECT->tracks[self->track_id];
  switch (self->type)
    {
    case EDIT_TRACK_ACTION_TYPE_SOLO:
      track->solo = self->solo_new;
      refresh_chan_and_track_widgets (
        self);
      break;
    case EDIT_TRACK_ACTION_TYPE_MUTE:
      track->mute = self->mute_new;
      refresh_chan_and_track_widgets (
        self);
      break;
    case EDIT_TRACK_ACTION_TYPE_RECORD:
      break;
    }
}

void
edit_track_action_undo (EditTrackAction * self)
{
  Track * track = PROJECT->tracks[self->track_id];
  switch (self->type)
    {
    case EDIT_TRACK_ACTION_TYPE_SOLO:
      track->solo = !self->solo_new;
      refresh_chan_and_track_widgets (
        self);
      break;
    case EDIT_TRACK_ACTION_TYPE_MUTE:
      track->mute = !self->mute_new;
      refresh_chan_and_track_widgets (
        self);
      break;
    case EDIT_TRACK_ACTION_TYPE_RECORD:
      break;
    }
}

void
edit_track_action_free (EditTrackAction * self)
{
  free (self);
}

