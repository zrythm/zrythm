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

#include "actions/edit_tracks_action.h"
#include "audio/channel.h"
#include "audio/track.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/flags.h"

#include <glib/gi18n.h>

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
  int                  mute_new)
{
  EditTracksAction * self =
    calloc (1, sizeof (EditTracksAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_EDIT_TRACKS;

  self->type = type;
  self->main_track_pos = main_track->pos;
  self->vol_delta = vol_delta;
  self->pan_delta = pan_delta;
  self->solo_new = solo_new;
  self->mute_new = mute_new;

  self->tls = tracklist_selections_clone (tls);

  return ua;
}

int
edit_tracks_action_do (EditTracksAction * self)
{
  int i;
  Track * track;
  Channel * ch;
  for (i = 0; i < self->tls->num_tracks; i++)
    {
      track =
        TRACKLIST->tracks[self->tls->tracks[i]->pos];
      g_return_val_if_fail (track, -1);
      ch = track->channel;

      switch (self->type)
        {
        case EDIT_TRACK_ACTION_TYPE_SOLO:
          track_set_soloed (
            track, self->solo_new, false,
            F_NO_PUBLISH_EVENTS);
          break;
        case EDIT_TRACK_ACTION_TYPE_MUTE:
          track_set_muted (
            track, self->mute_new,
            false, F_NO_PUBLISH_EVENTS);
          break;
        case EDIT_TRACK_ACTION_TYPE_VOLUME:
          g_return_val_if_fail (ch, -1);
          /* FIXME this is not really gonna work for
           * multi tracks */
          fader_add_amp (
            &ch->fader, self->vol_delta);
          break;
        case EDIT_TRACK_ACTION_TYPE_PAN:
          g_return_val_if_fail (ch, -1);
          /* FIXME this is not really gonna work for
           * multi tracks either */
          channel_add_balance_control (
            ch, self->pan_delta);
          break;
        }

      EVENTS_PUSH (ET_TRACK_STATE_CHANGED,
                   track);
    }

  return 0;
}

int
edit_tracks_action_undo (
  EditTracksAction * self)
{
  int i;
  Track * track;
  Channel * ch;
  for (i = 0; i < self->tls->num_tracks; i++)
    {
      track =
        TRACKLIST->tracks[self->tls->tracks[i]->pos];
      g_return_val_if_fail (track, -1);
      ch = track_get_channel (track);

      switch (self->type)
        {
        case EDIT_TRACK_ACTION_TYPE_SOLO:
          track_set_soloed (
            track, !self->solo_new, false,
            F_NO_PUBLISH_EVENTS);
          break;
        case EDIT_TRACK_ACTION_TYPE_MUTE:
          track_set_muted (
            track, !self->mute_new,
            false, F_NO_PUBLISH_EVENTS);
          break;
        case EDIT_TRACK_ACTION_TYPE_VOLUME:
          g_return_val_if_fail (ch, -1);
          /* FIXME this is not really gonna work for
           * multi tracks */
          fader_set_amp (
            &ch->fader, - self->vol_delta);
          break;
        case EDIT_TRACK_ACTION_TYPE_PAN:
          g_return_val_if_fail (ch, -1);
          /* FIXME this is not really gonna work for
           * multi tracks either */
          channel_add_balance_control (
            ch, - self->pan_delta);
          break;
        }
      EVENTS_PUSH (ET_TRACK_STATE_CHANGED,
                   track);
    }

  return 0;
}

char *
edit_tracks_action_stringize (
  EditTracksAction * self)
{
  if (self->tls->num_tracks == 1)
    {
      switch (self->type)
        {
        case EDIT_TRACK_ACTION_TYPE_SOLO:
          if (self->solo_new)
            return g_strdup (
              _("Solo Track"));
          else
            return g_strdup (
              _("Unsolo Track"));
        case EDIT_TRACK_ACTION_TYPE_MUTE:
          if (self->mute_new)
            return g_strdup (
              _("Mute Track"));
          else
            return g_strdup (
              _("Unmute Track"));
        case EDIT_TRACK_ACTION_TYPE_VOLUME:
          return g_strdup (
            _("Change Fader"));
        case EDIT_TRACK_ACTION_TYPE_PAN:
          return g_strdup (
            _("Change Pan"));
        default:
          g_return_val_if_reached (
            g_strdup (""));
        }
    }
  else
    g_return_val_if_reached (g_strdup (""));
}

void
edit_tracks_action_free (EditTracksAction * self)
{
  free (self);
}

