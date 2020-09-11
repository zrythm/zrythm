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
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
edit_tracks_action_init_loaded (
  EditTracksAction * self)
{
  tracklist_selections_init_loaded (
    self->tls_before);
  tracklist_selections_init_loaded (
    self->tls_after);
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

  self->tls_before =
    tracklist_selections_clone (tls);
  /* FIXME */
  self->tls_after =
    tracklist_selections_clone (tls);

  return ua;
}

/**
 * Generic edit action.
 */
UndoableAction *
edit_tracks_action_new_generic (
  EditTracksActionType  type,
  TracklistSelections * tls_before,
  TracklistSelections * tls_after,
  bool                  already_edited)
{
  EditTracksAction * self =
    object_new (EditTracksAction);
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_EDIT_TRACKS;

  self->type = type;
  self->tls_before =
    tracklist_selections_clone (tls_before);
  self->tls_after =
    tracklist_selections_clone (tls_after);

  self->already_edited = already_edited;

  return ua;
}

/**
 * Generic edit action.
 */
UndoableAction *
edit_tracks_action_new_track_float (
  EditTracksActionType  type,
  Track *               track,
  float                 val_before,
  float                 val_after,
  bool                  already_edited)
{
  TracklistSelections * sel_before =
    tracklist_selections_new (false);
  TracklistSelections * sel_after =
    tracklist_selections_new (false);
  Track * clone =
    track_clone (track, true);
  Track * clone_with_change =
    track_clone (track, true);

  if (type == EDIT_TRACK_ACTION_TYPE_VOLUME)
    {
      fader_set_amp (
        clone->channel->fader, val_before);
      fader_set_amp (
        clone_with_change->channel->fader,
        val_after);
    }
  else if (type == EDIT_TRACK_ACTION_TYPE_PAN)
    {
      channel_set_balance_control (
        clone->channel, val_before);
      channel_set_balance_control (
        clone_with_change->channel, val_after);
    }
  else
    {
      g_warn_if_reached ();
    }

  tracklist_selections_add_track (
    sel_before, clone, F_NO_PUBLISH_EVENTS);
  tracklist_selections_add_track (
    sel_after, clone_with_change,
    F_NO_PUBLISH_EVENTS);

  UndoableAction * ua =
    edit_tracks_action_new_generic (
      type, sel_before, sel_after, already_edited);

  tracklist_selections_free (sel_before);
  tracklist_selections_free (sel_after);

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
      EDIT_TRACK_ACTION_TYPE_SOLO, tls, NULL,
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

/**
 * Wrapper over edit_tracks_action_new().
 */
UndoableAction *
edit_tracks_action_new_color (
  TracklistSelections * tls,
  GdkRGBA *             color)
{
  EditTracksAction * self =
    object_new (EditTracksAction);

  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_EDIT_TRACKS;

  self->type = EDIT_TRACK_ACTION_TYPE_COLOR;
  self->new_color = *color;

  self->tls_before =
    tracklist_selections_clone (tls);
  /* FIXME */
  self->tls_after =
    tracklist_selections_clone (self->tls_before);

  return ua;
}

int
edit_tracks_action_do (EditTracksAction * self)
{
  if (self->already_edited)
    {
      self->already_edited = false;
      return 0;
    }

  int i;
  Channel * ch;
  for (i = 0; i < self->tls_before->num_tracks; i++)
    {
      Track * track =
        TRACKLIST->tracks[
          self->tls_before->tracks[i]->pos];
      Track * clone_track =
        self->tls_after->tracks[i];
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
          fader_set_amp (
            ch->fader,
            fader_get_amp (
              clone_track->channel->fader));
          break;
        case EDIT_TRACK_ACTION_TYPE_PAN:
          g_return_val_if_fail (ch, -1);
          channel_set_balance_control (
            ch,
            channel_get_balance_control (
              clone_track->channel));
          break;
        case EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
          g_return_val_if_fail (ch, -1);

          /* remove previous direct out */
          if (ch->has_output)
            {
              group_target_track_remove_child (
                TRACKLIST->tracks[ch->output_pos],
                ch->track->pos, F_DISCONNECT,
                F_RECALC_GRAPH, F_PUBLISH_EVENTS);
            }

          if (self->new_direct_out_pos != -1)
            {
              group_target_track_add_child (
                TRACKLIST->tracks[
                  self->new_direct_out_pos],
                ch->track->pos, F_CONNECT,
                F_RECALC_GRAPH, F_PUBLISH_EVENTS);
            }
          break;
        case EDIT_TRACK_ACTION_TYPE_RENAME:
          track_set_name (
            track, clone_track->name,
            F_NO_PUBLISH_EVENTS);

          /* remember the new name */
          g_free (clone_track->name);
          clone_track->name = g_strdup (track->name);
          break;
        case EDIT_TRACK_ACTION_TYPE_COLOR:
          track_set_color (
            track, &self->new_color,
            F_NOT_UNDOABLE, F_PUBLISH_EVENTS);
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
  for (i = 0; i < self->tls_after->num_tracks; i++)
    {
      Track * clone_track_before =
        self->tls_before->tracks[i];
      Track * clone_track_after =
        self->tls_after->tracks[i];
      Track * track =
        TRACKLIST->tracks[
          self->tls_after->tracks[i]->pos];
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
          fader_set_amp (
            ch->fader,
            fader_get_amp (
              clone_track_before->channel->fader));
          break;
        case EDIT_TRACK_ACTION_TYPE_PAN:
          g_return_val_if_fail (ch, -1);
          channel_set_balance_control (
            ch,
            channel_get_balance_control (
              clone_track_before->channel));
          break;
        case EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
          g_return_val_if_fail (ch, -1);

          /* disconnect from the current track */
          if (ch->has_output)
            {
              group_target_track_remove_child (
                TRACKLIST->tracks[ch->output_pos],
                ch->track->pos,
                F_DISCONNECT, F_RECALC_GRAPH,
                F_PUBLISH_EVENTS);
            }

          /* reconnect to the original track */
          if (clone_track_after->channel->has_output)
            {
              group_target_track_add_child (
                TRACKLIST->tracks[
                  clone_track_after->channel->output_pos],
                ch->track->pos, F_CONNECT,
                F_RECALC_GRAPH, F_PUBLISH_EVENTS);
            }
          break;
        case EDIT_TRACK_ACTION_TYPE_RENAME:
          track_set_name (
            track, clone_track_before->name,
            F_NO_PUBLISH_EVENTS);
          break;
        case EDIT_TRACK_ACTION_TYPE_COLOR:
          track_set_color (
            track, &clone_track_before->color,
            F_NOT_UNDOABLE, F_PUBLISH_EVENTS);
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
  if (self->tls_before->num_tracks == 1)
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
        case EDIT_TRACK_ACTION_TYPE_RENAME:
          return g_strdup (
            _("Rename track"));
        case EDIT_TRACK_ACTION_TYPE_COLOR:
          return g_strdup (
            _("Change color"));
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
  object_free_w_func_and_null (
    tracklist_selections_free, self->tls_before);
  object_free_w_func_and_null (
    tracklist_selections_free, self->tls_after);

  free (self);
}

