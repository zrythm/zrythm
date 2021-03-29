/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/tracklist_selections.h"
#include "audio/audio_region.h"
#include "audio/group_target_track.h"
#include "audio/midi_file.h"
#include "audio/router.h"
#include "audio/supported_file.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/algorithms.h"
#include "utils/arrays.h"
#include "utils/err_codes.h"
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
tracklist_selections_action_init_loaded (
  TracklistSelectionsAction * self)
{
  if (self->tls_before)
    {
      tracklist_selections_init_loaded (
        self->tls_before);
    }
  if (self->tls_after)
    {
      tracklist_selections_init_loaded (
        self->tls_after);
    }

  self->src_sends_size =
    (size_t) self->num_src_sends;

  for (int i = 0; i < self->num_src_sends; i++)
    {
      channel_send_init_loaded (
        self->src_sends[i], false);
    }
}

static void
copy_track_positions (
  TracklistSelections * sel,
  int *                 tracks,
  int *                 num_tracks)
{
  *num_tracks = sel->num_tracks;
  for (int i = 0; i < sel->num_tracks; i++)
    {
      tracks[i] = sel->tracks[i]->pos;
    }
  qsort (
    tracks, (size_t) *num_tracks, sizeof (int),
    algorithm_sort_int_cmpfunc);
}

/**
 * Validates the newly-created action.
 */
static bool
validate (
  TracklistSelectionsAction * self)
{
  if (self->type ==
        TRACKLIST_SELECTIONS_ACTION_DELETE)
    {
      if (!self->tls_before ||
          tracklist_selections_contains_undeletable_track (
            self->tls_before))
        {
          return false;
        }
    }

  return true;
}

/**
 * Creates a new TracklistSelectionsAction.
 *
 * @param tls Tracklist selections to act upon.
 * @param pos Position to make the tracks at.
 * @param pl_setting Plugin setting, if any.
 */
UndoableAction *
tracklist_selections_action_new (
  TracklistSelectionsActionType type,
  TracklistSelections *         tls_before,
  TracklistSelections *         tls_after,
  Track *                       track,
  TrackType                     track_type,
  PluginSetting *               pl_setting,
  SupportedFile *               file_descr,
  int                           track_pos,
  const Position *              pos,
  int                           num_tracks,
  EditTracksActionType          edit_type,
  Track *                       direct_out,
  int                           ival_after,
  const GdkRGBA *               color_new,
  float                         val_before,
  float                         val_after,
  const char *                  new_txt,
  bool                          already_edited)
{
  TracklistSelectionsAction * self =
    object_new (TracklistSelectionsAction);
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_TRACKLIST_SELECTIONS;

  position_init (&self->pos);

  if (num_tracks < 0)
    {
      num_tracks = 0;
    }

  self->type = type;
  if (pl_setting)
    {
      self->pl_setting =
        plugin_setting_clone (
          pl_setting, F_VALIDATE);
    }
  else if (file_descr)
    {
      self->file_descr =
        supported_file_clone (file_descr);
    }
  else
    {
      self->is_empty = 1;
    }
  self->track_type = track_type;
  self->track_pos = track_pos;
  if (type == TRACKLIST_SELECTIONS_ACTION_PIN)
    {
      self->track_pos =
        TRACKLIST->pinned_tracks_cutoff;
    }
  else if (type ==
             TRACKLIST_SELECTIONS_ACTION_UNPIN)
    {
      self->track_pos = TRACKLIST->num_tracks - 1;
    }

  self->pool_id = -1;
  if (pos)
    {
      position_set_to_pos (&self->pos, pos);
      self->have_pos = true;
    }

  /* calculate number of tracks */
  if (file_descr && track_type == TRACK_TYPE_MIDI)
    {
      self->num_tracks =
        midi_file_get_num_tracks (
          self->file_descr->abs_path, true);
    }
  else
    {
      self->num_tracks = num_tracks;
    }

  bool need_full_selections = true;
  if (type == TRACKLIST_SELECTIONS_ACTION_EDIT)
    {
      need_full_selections = false;
    }

  if (tls_before)
    {
      if (need_full_selections)
        {
          self->tls_before =
            tracklist_selections_clone (tls_before);
          tracklist_selections_sort (
            self->tls_before);
        }
      else
        {
          copy_track_positions (
            tls_before, self->tracks_before,
            &self->num_tracks);
        }
    }
  if (tls_after)
    {
      if (need_full_selections)
        {
          self->tls_after =
            tracklist_selections_clone (tls_after);
          tracklist_selections_sort (
            self->tls_after);
        }
      else
        {
          copy_track_positions (
            tls_after, self->tracks_after,
            &self->num_tracks);
        }
    }

  if (tls_before && need_full_selections)
    {
      /* save the incoming sends */
      for (int k = 0;
           k < self->tls_before->num_tracks; k++)
        {
          int clone_track_pos =
            self->tls_before->tracks[k]->pos;

          for (int i = 0; i < TRACKLIST->num_tracks;
               i++)
            {
              Track * cur_track =
                TRACKLIST->tracks[i];
              if (!track_type_has_channel (
                    cur_track->type))
                {
                  continue;
                }

              for (int j = 0; j < STRIP_SIZE; j++)
                {
                  ChannelSend * send =
                    cur_track->channel->sends[j];
                  if (channel_send_is_empty (send))
                    {
                      continue;
                    }

                  Track * target_track =
                    channel_send_get_target_track (
                      send);

                  if (target_track->pos ==
                        clone_track_pos)
                    {
                      array_double_size_if_full (
                        self->src_sends,
                        self->num_src_sends,
                        self->src_sends_size,
                        ChannelSend);
                      self->src_sends[
                        self->num_src_sends++] =
                          channel_send_clone (
                            send);
                    }
                }
            }
        }
    }

  self->edit_type = edit_type;
  self->ival_after = ival_after;
  self->new_direct_out_pos =
    direct_out ? direct_out->pos : -1;
  self->val_before = val_before;
  self->val_after = val_after;
  self->already_edited = already_edited;

  if (self->type ==
        TRACKLIST_SELECTIONS_ACTION_EDIT &&
      track)
    {
      self->num_tracks = 1;
      self->tracks_before[0] = track->pos;
      self->tracks_after[0] = track->pos;
    }

  if (color_new)
    {
      self->new_color = *color_new;
    }
  if (new_txt)
    {
      self->new_txt = g_strdup (new_txt);
    }

  g_return_val_if_fail (validate (self), NULL);

  return ua;
}

/**
 * @param add_to_project Used when the track to
 *   create is meant to be used in the project (ie
 *   not one of the tracks in the action).
 *
 * @return Non-zero if error.
 */
static int
create_track (
  TracklistSelectionsAction * self,
  int                         idx)
{
  Track * track;
  int pos = self->track_pos + idx;

  if (self->is_empty)
    {
      const char * track_type_str =
        track_stringize_type (self->track_type);
      char label[600];
      sprintf (
        label, _("%s Track"), track_type_str);

      track =
        track_new (
          self->track_type, pos, label, F_WITH_LANE);
      tracklist_insert_track (
        TRACKLIST, track, pos,
        F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
    }
  else // track is not empty
    {
      Plugin * pl = NULL;

      if (self->file_descr &&
          self->track_type == TRACK_TYPE_AUDIO)
        {
          char * basename =
            g_path_get_basename (
              self->file_descr->abs_path);
          track =
            track_new (
              self->track_type, pos, basename,
              F_WITH_LANE);
          g_free (basename);
        }
      else if (self->file_descr &&
               self->track_type == TRACK_TYPE_MIDI)
        {
          char * basename =
            g_path_get_basename (
              self->file_descr->abs_path);
          track =
            track_new (
              self->track_type, pos, basename,
              F_WITH_LANE);
          g_free (basename);
        }
      /* at this point we can assume it has a
       * plugin */
      else
        {
          track =
            track_new (
              self->track_type, pos,
              self->pl_setting->descr->name,
              F_WITH_LANE);

          pl=
            plugin_new_from_setting (
              self->pl_setting, track->pos,
              PLUGIN_SLOT_INSERT, 0);
          if (!pl)
            {
              g_warning (
                "plugin instantiation failed");
              return ERR_PLUGIN_INSTANTIATION_FAILED;
            }

          if (plugin_instantiate (pl, true, NULL) != 0)
            {
              char * message =
                g_strdup_printf (
                  _("Error instantiating plugin %s. "
                    "Please see log for details."),
                  pl->setting->descr->name);

              if (MAIN_WINDOW)
                {
                  ui_show_error_message (
                    GTK_WINDOW (MAIN_WINDOW),
                    message);
                }
              g_free (message);
              plugin_free (pl);
              return
                ERR_PLUGIN_INSTANTIATION_FAILED;
            }

          /* activate */
          g_return_val_if_fail (
            plugin_activate (pl, F_ACTIVATE) == 0,
            -1);
        }

      tracklist_insert_track (
        TRACKLIST, track, track->pos,
        F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);

      if (track->channel && pl)
        {
          channel_add_plugin (
            track->channel,
            plugin_descriptor_is_instrument (
              self->pl_setting->descr) ?
                PLUGIN_SLOT_INSTRUMENT :
                PLUGIN_SLOT_INSERT,
            pl->id.slot, pl,
            F_CONFIRM, F_NOT_MOVING_PLUGIN,
            F_GEN_AUTOMATABLES, F_NO_RECALC_GRAPH,
            F_NO_PUBLISH_EVENTS);
          g_warn_if_fail (
            pl->id.track_pos == track->pos);
        }

      Position start_pos;
      position_init (&start_pos);
      if (self->have_pos)
        {
          position_set_to_pos (
            &start_pos, &self->pos);
        }
      if (self->track_type == TRACK_TYPE_AUDIO)
        {
          /* create an audio region & add to
           * track */
          ZRegion * ar =
            audio_region_new (
              self->pool_id,
              self->pool_id == -1 ?
                self->file_descr->abs_path :
                NULL,
              NULL, 0, NULL, 0,
              &start_pos, pos, 0, 0);
          self->pool_id =
            ar->pool_id;
          track_add_region (
            track, ar, NULL, 0, F_GEN_NAME,
            F_NO_PUBLISH_EVENTS);
        }
      else if (self->track_type == TRACK_TYPE_MIDI &&
               self->file_descr)
        {
          /* create a MIDI region from the MIDI
           * file & add to track */
          ZRegion * mr =
            midi_region_new_from_midi_file (
              &start_pos,
              self->file_descr->abs_path,
              pos, 0, 0, idx);
          if (mr)
            {
              track_add_region (
                track, mr, NULL, 0,
                /* name could already be generated
                 * based
                 * on the track name (if any) in
                 * the MIDI file */
                mr->name ?
                  F_NO_GEN_NAME : F_GEN_NAME,
                F_NO_PUBLISH_EVENTS);
            }
          else
            {
              g_message (
                "Failed to create MIDI region from "
                "file %s",
                self->file_descr->abs_path);
            }
        }

      if (pl)
        {
          g_return_val_if_fail (
            pl->instantiated, -1);
        }

      if (pl && ZRYTHM_HAVE_UI &&
          g_settings_get_boolean (
            S_P_PLUGINS_UIS,
            "open-on-instantiate"))
        {
          pl->visible = 1;
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, pl);
        }
    }

  return 0;
}

/**
 * Reconnects the given send.
 *
 * @note The given send must be the send on the
 *   project track (not a clone).
 */
static void
reconnect_send (
  ChannelSend * send)
{
  Track * src = TRACKLIST->tracks[send->track_pos];

  if (src->out_signal_type == TYPE_AUDIO)
    {
      Port * l =
        port_find_from_identifier (
          &send->dest_l_id);
      Port * r =
        port_find_from_identifier (
          &send->dest_r_id);
      channel_send_connect_stereo (
        send, NULL, l, r, send->is_sidechain);
    }
  else if (src->out_signal_type == TYPE_EVENT)
    {
      Port * midi =
        port_find_from_identifier (
          &send->dest_midi_id);
      channel_send_connect_midi (send, midi);
    }
}

static int
do_or_undo_create_or_delete (
  TracklistSelectionsAction * self,
  bool                        _do,
  bool                        create)
{
  /* if creating tracks (do create or undo delete) */
  if ((create && _do) ||
      (!create && !_do))
    {
      if (create)
        {
          for (int i = 0; i < self->num_tracks; i++)
            {
              int ret = create_track (self, i);
              if (ret != 0)
                {
                  g_warn_if_reached ();
                  return ret;
                }

              /* TODO select each plugin that was
               * selected */
            }
        }
      /* else if delete undo */
      else
        {
          for (int i = 0;
               i < self->tls_before->num_tracks; i++)
            {
              Track * own_track =
                self->tls_before->tracks[i];

              /* clone our own track */
              Track * track =
                track_clone (own_track, false);

              /* insert it to the tracklist at its
               * original pos */
              track->num_children = 0;
              tracklist_insert_track (
                TRACKLIST, track, track->pos,
                F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);

              /* if group track, readd all children */
              if (TRACK_CAN_BE_GROUP_TARGET (track))
                {
                  group_target_track_add_children (
                    track, own_track->children,
                    own_track->num_children,
                    F_DISCONNECT, F_NO_RECALC_GRAPH,
                    F_NO_PUBLISH_EVENTS);
                }

              if (track->type == TRACK_TYPE_INSTRUMENT)
                {
                  if (own_track->channel->
                        instrument->visible)
                    {
                      EVENTS_PUSH (
                        ET_PLUGIN_VISIBILITY_CHANGED,
                        track->channel->instrument);
                    }
                }
            }

          for (int i = 0;
               i < self->tls_before->num_tracks; i++)
            {
              Track * own_track =
                self->tls_before->tracks[i];

              /* get the project track */
              Track * track =
                TRACKLIST->tracks[own_track->pos];
              if (!track_type_has_channel (
                     track->type))
                continue;

              /* reconnect any sends sent from the
               * track */
              for (int j = 0; j < STRIP_SIZE; j++)
                {
                  ChannelSend * clone_send =
                    own_track->channel->sends[j];
                  ChannelSend * send =
                    track->channel->sends[j];
                  channel_send_copy_values (
                    send, clone_send);
                  if (channel_send_is_empty (
                        clone_send))
                    continue;

                  reconnect_send (send);
                }

              /* reconnect any custom connections */
              size_t max_size = 20;
              int num_ports = 0;
              Port ** ports =
                object_new_n (max_size, Port *);
              track_append_all_ports (
                own_track,
                &ports, &num_ports, true, &max_size,
                true);
              for (int j = 0; j < num_ports; j++)
                {
                  Port * port = ports[j];
                  Port * prj_port =
                    port_find_from_identifier (
                      &port->id);
                  port_restore_from_non_project (
                    prj_port, port);
                }
              free (ports);

              track_validate (track);
            }

          /* re-connect any source sends */
          for (int i = 0; i < self->num_src_sends; i++)
            {
              ChannelSend * clone_send =
                self->src_sends[i];

              /* get the original send and connect it */
              Track * orig_track =
                TRACKLIST->tracks[
                  clone_send->track_pos];
              ChannelSend * send =
                orig_track->channel->sends[
                  clone_send->slot];

              reconnect_send (send);

              track_validate (orig_track);
            }
        }

      EVENTS_PUSH (ET_TRACKS_ADDED, NULL);
    }
  /* else if deleting tracks (delete do or create
   * undo) */
  else
    {
      /* if create undo */
      if (create)
        {
          for (int i = self->num_tracks - 1;
               i >= 0; i--)
            {
              Track * track =
                TRACKLIST->tracks[
                  self->track_pos + i];
              g_return_val_if_fail (track, -1);

              tracklist_remove_track (
                TRACKLIST, track,
                F_REMOVE_PL, F_FREE,
                F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);
            }
        }
      /* else if delete do */
      else
        {
          /* remove any sends pointing to any track */
          for (int i = 0; i < self->num_src_sends;
               i++)
            {
              ChannelSend * clone_send =
                self->src_sends[i];

              /* get the original send and disconnect
               * it */
              ChannelSend * send =
                TRACKLIST->tracks[
                  clone_send->track_pos]->
                    channel->sends[clone_send->slot];
              channel_send_disconnect (send);
            }

          for (int i =
                 self->tls_before->num_tracks - 1;
               i >= 0; i--)
            {
              Track * own_track =
                self->tls_before->tracks[i];

              /* get track from pos */
              Track * track =
                TRACKLIST->tracks[own_track->pos];
              g_return_val_if_fail (track, -1);

              /* remember any custom connections */
              size_t max_size = 20;
              int num_ports = 0;
              Port ** ports =
                object_new_n (max_size, Port *);
              track_append_all_ports (
                track, &ports, &num_ports, true,
                &max_size, true);
              max_size = 20;
              int num_clone_ports = 0;
              Port ** clone_ports =
                object_new_n (max_size, Port *);
              track_append_all_ports (
                own_track, &clone_ports,
                &num_clone_ports,
                true, &max_size, true);
              for (int j = 0; j < num_ports; j++)
                {
                  Port * prj_port = ports[j];

                  Port * clone_port = NULL;
                  for (int k = 0;
                       k < num_clone_ports; k++)
                    {
                      Port * cur_clone_port =
                        clone_ports[k];
                      if (port_identifier_is_equal (
                            &cur_clone_port->id,
                            &prj_port->id))
                        {
                          clone_port =
                            cur_clone_port;
                          break;
                        }
                    }
                  g_return_val_if_fail (
                    clone_port, -1);

                  port_copy_metadata_from_project (
                    clone_port, prj_port);
                }
              free (ports);
              free (clone_ports);

              /* if group track, remove all children */
              if (TRACK_CAN_BE_GROUP_TARGET (track))
                {
                  group_target_track_remove_all_children (
                    track, F_DISCONNECT,
                    F_NO_RECALC_GRAPH,
                    F_NO_PUBLISH_EVENTS);
                }

              /* remove it */
              tracklist_remove_track (
                TRACKLIST, track, F_REMOVE_PL,
                F_FREE, F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);
            }
        }

      EVENTS_PUSH (ET_TRACKS_REMOVED, NULL);
      EVENTS_PUSH (
        ET_CLIP_EDITOR_REGION_CHANGED, NULL);
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  return 0;
}

static int
do_or_undo_move_or_copy (
  TracklistSelectionsAction * self,
  bool                        _do,
  bool                        copy)
{
  bool move = !copy;
  bool pin =
    self->type == TRACKLIST_SELECTIONS_ACTION_PIN;
  bool unpin =
    self->type == TRACKLIST_SELECTIONS_ACTION_UNPIN;

  if (_do)
    {
      if (move)
        {
          /* get the project tracks */
          Track * prj_tracks[4000];
          for (int i = 0;
               i < self->tls_before->num_tracks; i++)
            {
              prj_tracks[i] =
                TRACKLIST->tracks[
                  self->tls_before->tracks[i]->pos];
            }

          for (int i =
               self->tls_before->num_tracks - 1;
               i >= 0; i--)
            {
              Track * prj_track = prj_tracks[i];
              g_return_val_if_fail (prj_track, -1);

              tracklist_move_track (
                TRACKLIST, prj_track,
                self->track_pos,
                F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);

              if (i == 0)
                tracklist_selections_select_single (
                  TRACKLIST_SELECTIONS, prj_track,
                  F_NO_PUBLISH_EVENTS);
              else
                tracklist_selections_add_track (
                  TRACKLIST_SELECTIONS, prj_track, 0);
            }

          EVENTS_PUSH (ET_TRACKS_MOVED, NULL);
        }
      else if (copy)
        {
          for (int i = 0;
               i < self->tls_before->num_tracks;
               i++)
            {
              Track * own_track =
                self->tls_before->tracks[i];

              /* create a new clone to use in the
               * project */
              Track * track =
                track_clone (own_track, false);

              /* add to tracklist at given pos */
              tracklist_insert_track (
                TRACKLIST, track,
                self->track_pos + i,
                F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);

              /* select it */
              track_select (
                track, F_SELECT, 1,
                F_NO_PUBLISH_EVENTS);
            }

          EVENTS_PUSH (ET_TRACK_ADDED, NULL);
          EVENTS_PUSH (
            ET_TRACKLIST_SELECTIONS_CHANGED, NULL);
        }
    }
  /* if undoing */
  else
    {
      if (move)
        {
          for (int i =
                 self->tls_before->num_tracks - 1;
               i >= 0; i--)
            {
              Track * own_track =
                self->tls_before->tracks[i];

              Track * prj_track =
                TRACKLIST->tracks[
                  self->track_pos];
              g_return_val_if_fail (prj_track, -1);
              g_return_val_if_fail (
                own_track->pos != prj_track->pos,
                -1);

              tracklist_move_track (
                TRACKLIST, prj_track,
                own_track->pos,
                F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);

              if (i == 0)
                {
                  tracklist_selections_select_single (
                    TRACKLIST_SELECTIONS, prj_track,
                    F_NO_PUBLISH_EVENTS);
                }
              else
                {
                  tracklist_selections_add_track (
                    TRACKLIST_SELECTIONS,
                    prj_track, 0);
                }
            }

          EVENTS_PUSH (ET_TRACKS_MOVED, NULL);
        }
      else if (copy)
        {
          for (int i =
                 self->tls_before->num_tracks - 1;
               i >= 0; i--)
            {
              /* get the track from the inserted
               * pos */
              Track * track =
                TRACKLIST->tracks[
                  self->track_pos + i];
              g_return_val_if_fail (track, -1);

              /* remove it */
              tracklist_remove_track (
                TRACKLIST, track, F_REMOVE_PL,
                F_FREE, F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);
            }
          EVENTS_PUSH (
            ET_TRACKLIST_SELECTIONS_CHANGED, NULL);
          EVENTS_PUSH (ET_TRACKS_REMOVED, NULL);
        }
    }

  if ((pin && _do) ||
       (unpin && !_do))
    {
      TRACKLIST->pinned_tracks_cutoff +=
         self->tls_before->num_tracks;
    }
  else if ((unpin && _do) ||
            (pin && !_do))
    {
      TRACKLIST->pinned_tracks_cutoff -=
         self->tls_before->num_tracks;
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  return 0;
}

static int
do_or_undo_edit (
  TracklistSelectionsAction * self,
  bool                        _do)
{
  if (_do && self->already_edited)
    {
      self->already_edited = false;
      return 0;
    }

  int * tracks = self->tracks_before;
  int num_tracks = self->num_tracks;

  for (int i = 0; i < num_tracks; i++)
    {
      Track * track =
        TRACKLIST->tracks[tracks[i]];
      g_return_val_if_fail (track, -1);
      Channel * ch = track->channel;

      switch (self->edit_type)
        {
        case EDIT_TRACK_ACTION_TYPE_SOLO:
          track_set_soloed (
            track, _do == self->ival_after,
            false,
            F_NO_PUBLISH_EVENTS);
          break;
        case EDIT_TRACK_ACTION_TYPE_MUTE:
          track_set_muted (
            track, _do == self->ival_after,
            F_NO_TRIGGER_UNDO, F_NO_PUBLISH_EVENTS);
          break;
        case EDIT_TRACK_ACTION_TYPE_VOLUME:
          g_return_val_if_fail (ch, -1);
          fader_set_amp (
            ch->fader,
            _do ?
              self->val_after : self->val_before);
          break;
        case EDIT_TRACK_ACTION_TYPE_PAN:
          g_return_val_if_fail (ch, -1);
          channel_set_balance_control (
            ch,
            _do ?
              self->val_after : self->val_before);
          break;
        case EDIT_TRACK_ACTION_TYPE_MIDI_FADER_MODE:
          g_return_val_if_fail (ch, -1);
          if (_do)
            {
              self->ival_before =
                ch->fader->midi_mode;
              fader_set_midi_mode (
                ch->fader, self->ival_after, false,
                F_PUBLISH_EVENTS);
            }
          else
            {
              fader_set_midi_mode (
                ch->fader, self->ival_before, false,
                F_PUBLISH_EVENTS);
            }
          break;
        case EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
          {
            g_return_val_if_fail (ch, -1);

            int cur_direct_out =
              ch->has_output ? ch->output_pos : -1;

            /* disconnect from the current track */
            if (ch->has_output)
              {
                group_target_track_remove_child (
                  TRACKLIST->tracks[ch->output_pos],
                  ch->track->pos, F_DISCONNECT,
                  F_RECALC_GRAPH,
                  F_PUBLISH_EVENTS);
              }

            /* reconnect to the new track */
            if (self->new_direct_out_pos != -1)
              {
                int target_pos =
                  self->new_direct_out_pos;
                g_return_val_if_fail (
                  target_pos != ch->track->pos, -1);
                group_target_track_add_child (
                  TRACKLIST->tracks[target_pos],
                  ch->track->pos, F_CONNECT,
                  F_RECALC_GRAPH, F_PUBLISH_EVENTS);
              }

            /* remember previous pos */
            self->new_direct_out_pos =
              cur_direct_out;
          }
          break;
        case EDIT_TRACK_ACTION_TYPE_RENAME:
          {
            char * cur_name =
              g_strdup (track->name);
            track_set_name (
              track, self->new_txt,
              F_NO_PUBLISH_EVENTS);

            /* remember the new name */
            g_free (self->new_txt);
            self->new_txt = cur_name;
          }
          break;
        case EDIT_TRACK_ACTION_TYPE_COLOR:
          {
            GdkRGBA cur_color = track->color;
            track_set_color (
              track, &self->new_color,
              F_NOT_UNDOABLE, F_PUBLISH_EVENTS);

            /* remember color */
            self->new_color = cur_color;
          }
          break;
        case EDIT_TRACK_ACTION_TYPE_ICON:
          {
            char * cur_icon =
              g_strdup (track->icon_name);
            track_set_icon (
              track, self->new_txt,
              F_NOT_UNDOABLE, F_PUBLISH_EVENTS);

            g_free (self->new_txt);
            self->new_txt = cur_icon;
          }
          break;
        case EDIT_TRACK_ACTION_TYPE_COMMENT:
          {
            char * cur_comment =
              g_strdup (track->comment);
            track_set_comment (
              track, self->new_txt,
              F_NOT_UNDOABLE);

            g_free (self->new_txt);
            self->new_txt = cur_comment;
          }
          break;
        }

      if (ZRYTHM_TESTING ||
          self->edit_type ==
            EDIT_TRACK_ACTION_TYPE_DIRECT_OUT)
        {
          track_validate (track);
        }

      EVENTS_PUSH (ET_TRACK_STATE_CHANGED, track);
    }

  return 0;
}

static int
do_or_undo (
  TracklistSelectionsAction * self,
  bool                        _do)
{
  switch (self->type)
    {
    case TRACKLIST_SELECTIONS_ACTION_COPY:
      return
        do_or_undo_move_or_copy (
          self, _do, true);
    case TRACKLIST_SELECTIONS_ACTION_CREATE:
      return
        do_or_undo_create_or_delete (
          self, _do, true);
    case TRACKLIST_SELECTIONS_ACTION_DELETE:
      return
        do_or_undo_create_or_delete (
          self, _do, false);
    case TRACKLIST_SELECTIONS_ACTION_EDIT:
      return do_or_undo_edit (self, _do);
    case TRACKLIST_SELECTIONS_ACTION_MOVE:
    case TRACKLIST_SELECTIONS_ACTION_PIN:
    case TRACKLIST_SELECTIONS_ACTION_UNPIN:
      return
        do_or_undo_move_or_copy (
          self, _do, false);
    }

  g_return_val_if_reached (-1);
}

int
tracklist_selections_action_do (
  TracklistSelectionsAction * self)
{
  return do_or_undo (self, true);
}

int
tracklist_selections_action_undo (
  TracklistSelectionsAction * self)
{
  return do_or_undo (self, false);
}

char *
tracklist_selections_action_stringize (
  TracklistSelectionsAction * self)
{
  switch (self->type)
    {
    case TRACKLIST_SELECTIONS_ACTION_COPY:
      if (self->tls_before->num_tracks == 1)
        {
          return g_strdup (
            _("Copy Track"));
        }
      else
        {
          return g_strdup_printf (
            _("Move %d Tracks"),
            self->tls_before->num_tracks);
        }
    case TRACKLIST_SELECTIONS_ACTION_CREATE:
      {
        const char * type =
          track_stringize_type (self->track_type);
        if (self->num_tracks == 1)
          {
            return
              g_strdup_printf (
                _("Create %s Track"), type);
          }
        else
          {
            return
              g_strdup_printf (
                _("Create %d %s Tracks"),
                self->num_tracks, type);
          }
      }
    case TRACKLIST_SELECTIONS_ACTION_DELETE:
      if (self->tls_before->num_tracks == 1)
        {
          return g_strdup (_("Delete Track"));
        }
      else
        {
          return g_strdup_printf (
            _("Delete %d Tracks"),
            self->tls_before->num_tracks);
        }
    case TRACKLIST_SELECTIONS_ACTION_EDIT:
      if ((self->tls_before &&
           self->tls_before->num_tracks == 1) ||
          (self->num_tracks == 1))
        {
          switch (self->edit_type)
            {
            case EDIT_TRACK_ACTION_TYPE_SOLO:
              if (self->ival_after)
                return g_strdup (
                  _("Solo Track"));
              else
                return g_strdup (
                  _("Unsolo Track"));
            case EDIT_TRACK_ACTION_TYPE_MUTE:
              if (self->ival_after)
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
            case EDIT_TRACK_ACTION_TYPE_ICON:
              return g_strdup (
                _("Change icon"));
            case EDIT_TRACK_ACTION_TYPE_COMMENT:
              return g_strdup (
                _("Change comment"));
            case EDIT_TRACK_ACTION_TYPE_MIDI_FADER_MODE:
              return g_strdup (
                _("Change MIDI fader mode"));
            default:
              g_return_val_if_reached (
                g_strdup (""));
            }
        }
      else
        {
          g_return_val_if_reached (g_strdup (""));
        }
    case TRACKLIST_SELECTIONS_ACTION_MOVE:
      if (self->tls_before->num_tracks == 1)
        {
          return g_strdup (
            _("Move Track"));
        }
      else
        {
          return g_strdup_printf (
            _("Move %d Tracks"),
            self->tls_before->num_tracks);
        }
      break;
    case TRACKLIST_SELECTIONS_ACTION_PIN:
      if (self->tls_before->num_tracks == 1)
        {
          return g_strdup (
            _("Pin Track"));
        }
      else
        {
          return g_strdup_printf (
            _("Pin %d Tracks"),
            self->tls_before->num_tracks);
        }
    case TRACKLIST_SELECTIONS_ACTION_UNPIN:
      if (self->tls_before->num_tracks == 1)
        {
          return g_strdup (
            _("Unpin Track"));
        }
      else
        {
          return g_strdup_printf (
            _("Unpin %d Tracks"),
            self->tls_before->num_tracks);
        }
      break;
    }

  g_return_val_if_reached (g_strdup (""));
}

void
tracklist_selections_action_free (
  TracklistSelectionsAction * self)
{
  object_free_w_func_and_null (
    tracklist_selections_free, self->tls_before);
  object_free_w_func_and_null (
    tracklist_selections_free, self->tls_after);
  object_free_w_func_and_null (
    plugin_setting_free, self->pl_setting);

  for (int i = 0; i < self->num_src_sends; i++)
    {
      object_free_w_func_and_null (
        channel_send_free, self->src_sends[i]);
    }
  object_zero_and_free (self->src_sends);

  object_zero_and_free (self);
}
