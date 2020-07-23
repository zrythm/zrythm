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

#include "actions/edit_tracks_action.h"
#include "audio/channel.h"
#include "audio/group_target_track.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
edit_tracks_action_init_loaded (
  EditTracksAction * self)
{
  tracklist_selections_init_loaded (self->tls);
}

/**
 * All-in-one constructor.
 *
 * Only the necessary params should be passed, others
 * will get ignored.
 */
UndoableAction *
edit_tracks_action_new (
  EditTracksActionType  type,
  TracklistSelections * tls,
  Track *               direct_out,
  float                 vol_delta,
  float                 pan_delta,
  bool                  solo_new,
  bool                  mute_new)
{
  EditTracksAction * self =
    calloc (1, sizeof (EditTracksAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_EDIT_TRACKS;

  self->type = type;
  self->vol_delta = vol_delta;
  self->pan_delta = pan_delta;
  self->solo_new = solo_new;
  self->mute_new = mute_new;
  self->new_direct_out_pos =
    direct_out ? direct_out->pos : -1;

  self->tls = tracklist_selections_clone (tls);

  return ua;
}

/**
 * Wrapper over edit_tracks_action_new().
 */
UndoableAction *
edit_tracks_action_new_mute (
  TracklistSelections * tls,
  bool                  mute_new)
{
  UndoableAction * action =
    edit_tracks_action_new (
      EDIT_TRACK_ACTION_TYPE_MUTE, tls, NULL,
      0.f, 0.f, false, mute_new);
  return action;
}

/**
 * Wrapper over edit_tracks_action_new().
 */
UndoableAction *
edit_tracks_action_new_solo (
  TracklistSelections * tls,
  bool                  solo_new)
{
  UndoableAction * action =
    edit_tracks_action_new (
      EDIT_TRACK_ACTION_TYPE_MUTE, tls, NULL,
      0.f, 0.f, solo_new, false);
  return action;
}

/**
 * Wrapper over edit_tracks_action_new().
 */
UndoableAction *
edit_tracks_action_new_direct_out (
  TracklistSelections * tls,
  Track *               direct_out)
{
  UndoableAction * action =
    edit_tracks_action_new (
      EDIT_TRACK_ACTION_TYPE_DIRECT_OUT, tls,
      direct_out,
      0.f, 0.f, false, false);
  return action;
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
            F_NO_TRIGGER_UNDO, F_NO_PUBLISH_EVENTS);
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
        case EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
          g_return_val_if_fail (ch, -1);
          if (self->new_direct_out_pos == -1)
            {
              group_target_track_remove_child (
                TRACKLIST->tracks[ch->output_pos],
                ch->track->pos, F_DISCONNECT,
                F_RECALC_GRAPH, F_PUBLISH_EVENTS);
            }
          else
            {
              group_target_track_add_child (
                TRACKLIST->tracks[
                  self->new_direct_out_pos],
                ch->track->pos, F_CONNECT,
                F_RECALC_GRAPH, F_PUBLISH_EVENTS);
            }
          break;
        }

      EVENTS_PUSH (ET_TRACK_STATE_CHANGED, track);
    }

  return 0;
}

int
edit_tracks_action_undo (
  EditTracksAction * self)
{
  int i;
  Channel * ch;
  for (i = 0; i < self->tls->num_tracks; i++)
    {
      Track * clone_track = self->tls->tracks[i];
      Track * track =
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
        case EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
          g_return_val_if_fail (ch, -1);
          if (clone_track->channel->has_output)
            {
              group_target_track_add_child (
                TRACKLIST->tracks[
                  clone_track->channel->output_pos],
                ch->track->pos, F_CONNECT,
                F_RECALC_GRAPH, F_PUBLISH_EVENTS);
            }
          else
            {
              group_target_track_remove_child (
                TRACKLIST->tracks[ch->output_pos],
                ch->track->pos,
                F_DISCONNECT, F_RECALC_GRAPH,
                F_PUBLISH_EVENTS);
            }
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
        case EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
          return g_strdup (
            _("Change direct out"));
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

