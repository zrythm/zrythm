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

#include "actions/delete_tracks_action.h"
#include "audio/group_target_track.h"
#include "audio/router.h"
#include "audio/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/tracklist_selections.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
delete_tracks_action_init_loaded (
  DeleteTracksAction * self)
{
  tracklist_selections_init_loaded (self->tls);
  self->src_sends_size = self->num_src_sends;
}

UndoableAction *
delete_tracks_action_new (
  TracklistSelections * tls)
{
  DeleteTracksAction * self =
    object_new (DeleteTracksAction);

  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UA_DELETE_TRACKS;

  self->tls = tracklist_selections_clone (tls);
  tracklist_selections_sort (self->tls);

  /* save the incoming sends */
  for (int k = 0; k < self->tls->num_tracks; k++)
    {
      int clone_track_pos =
        self->tls->tracks[k]->pos;

      for (int i = 0; i < TRACKLIST->num_tracks; i++)
        {
          Track * track = TRACKLIST->tracks[i];
          if (!track_type_has_channel (track->type))
            continue;

          for (int j = 0; j < STRIP_SIZE; j++)
            {
              ChannelSend * send =
                &track->channel->sends[j];
              if (send->is_empty)
                continue;

              Track * target_track =
                channel_send_get_target_track (send);

              if (target_track->pos ==
                    clone_track_pos)
                {
                  array_double_size_if_full (
                    self->src_sends, self->num_src_sends,
                    self->src_sends_size, ChannelSend);
                  self->src_sends[
                    self->num_src_sends++] =
                      *send;
                }
            }
        }
    }

  return ua;
}

int
delete_tracks_action_do (
  DeleteTracksAction * self)
{
  Track * track;

  /* remove any sends pointing to any track */
  for (int i = 0; i < self->num_src_sends; i++)
    {
      ChannelSend * clone_send = &self->src_sends[i];

      /* get the original send and disconnect it */
      ChannelSend * send =
        &TRACKLIST->tracks[clone_send->track_pos]->
          channel->sends[clone_send->slot];
      channel_send_disconnect (send);
    }

  for (int i = self->tls->num_tracks - 1;
       i >= 0; i--)
    {
      /* get track from pos */
      track =
        TRACKLIST->tracks[
          self->tls->tracks[i]->pos];
      g_return_val_if_fail (track, -1);

      /* remember any custom connections */
      int max_size = 20;
      int num_ports = 0;
      Port ** ports =
        calloc ((size_t) max_size, sizeof (Port *));
      track_append_all_ports (
        track, &ports, &num_ports, true, &max_size,
        true);
      max_size = 20;
      int num_clone_ports = 0;
      Port ** clone_ports =
        calloc ((size_t) max_size, sizeof (Port *));
      track_append_all_ports (
        self->tls->tracks[i], &clone_ports,
        &num_clone_ports,
        true, &max_size, true);
      for (int j = 0; j < num_ports; j++)
        {
          Port * prj_port = ports[j];

          Port * clone_port = NULL;
          for (int k = 0; k < num_clone_ports; k++)
            {
              Port * cur_clone_port = clone_ports[k];
              if (port_identifier_is_equal (
                    &cur_clone_port->id,
                    &prj_port->id))
                {
                  clone_port = cur_clone_port;
                  break;
                }
            }
          g_return_val_if_fail (clone_port, -1);

          clone_port->num_srcs = prj_port->num_srcs;
          clone_port->num_dests =
            prj_port->num_dests;
          /*char port_label[600];*/
          /*port_get_full_designation (*/
            /*clone_port, port_label);*/
          /*g_message (*/
            /*"clone port: %s %d %d", port_label,*/
            /*clone_port->num_srcs,*/
            /*clone_port->num_dests);*/
          for (int k = 0; k < prj_port->num_srcs;
               k++)
            {
              Port * src_port = prj_port->srcs[k];
              port_identifier_copy (
                &clone_port->src_ids[k],
                &src_port->id);
              clone_port->src_multipliers[k] =
                prj_port->src_multipliers[k];
              clone_port->src_enabled[k] =
                prj_port->src_enabled[k];
              clone_port->src_locked[k] =
                prj_port->src_locked[k];
            }
          for (int k = 0; k < prj_port->num_dests;
               k++)
            {
              Port * dest_port = prj_port->dests[k];
              port_identifier_copy (
                &clone_port->dest_ids[k],
                &dest_port->id);
              clone_port->multipliers[k] =
                prj_port->multipliers[k];
              clone_port->dest_enabled[k] =
                prj_port->dest_enabled[k];
              clone_port->dest_locked[k] =
                prj_port->dest_locked[k];
            }
        }
      free (ports);
      free (clone_ports);

      /* if group track, remove all children */
      if (TRACK_CAN_BE_GROUP_TARGET (track))
        {
          group_target_track_remove_all_children (
            track, F_DISCONNECT, F_NO_RECALC_GRAPH,
            F_NO_PUBLISH_EVENTS);
        }

      /* remove it */
      g_message (
        "removing track %s...", track->name);
      tracklist_remove_track (
        TRACKLIST, track, F_REMOVE_PL,
        F_FREE, F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);
    }

  EVENTS_PUSH (ET_TRACKS_REMOVED, NULL);
  EVENTS_PUSH (ET_CLIP_EDITOR_REGION_CHANGED, NULL);

  if (!ZRYTHM_TESTING)
    {
      /* process the events now */
      event_manager_process_now (EVENT_MANAGER);
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

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
        send, NULL, l, r);
    }
  else if (src->out_signal_type == TYPE_EVENT)
    {
      Port * midi =
        port_find_from_identifier (
          &send->dest_midi_id);
      channel_send_connect_midi (send, midi);
    }
}

/**
 * Undo deletion, create tracks.
 */
int
delete_tracks_action_undo (
  DeleteTracksAction * self)
{
  for (int i = 0; i < self->tls->num_tracks; i++)
    {
      /* clone the clone */
      Track * track =
        track_clone (self->tls->tracks[i], false);

      /* insert it to the tracklist at its original
       * pos */
      tracklist_insert_track (
        TRACKLIST, track, track->pos,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      /* if group track, readd all children */
      if (TRACK_CAN_BE_GROUP_TARGET (track))
        {
          track->num_children = 0;
          group_target_track_add_children (
            track, self->tls->tracks[i]->children,
            self->tls->tracks[i]->num_children,
            F_DISCONNECT, F_NO_RECALC_GRAPH,
            F_NO_PUBLISH_EVENTS);
        }

      if (track->type == TRACK_TYPE_INSTRUMENT)
        {
          if (track->channel->instrument->visible)
            {
              EVENTS_PUSH (
                ET_PLUGIN_VISIBILITY_CHANGED,
                track->channel->instrument);
            }
        }
    }

  for (int i = 0; i < self->tls->num_tracks; i++)
    {
      /* get the project track */
      Track * track =
        TRACKLIST->tracks[self->tls->tracks[i]->pos];
      if (!track_type_has_channel (track->type))
        continue;

      /* reconnect any sends sent from the track */
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          ChannelSend * clone_send =
            &self->tls->tracks[i]->channel->sends[j];
          ChannelSend * send =
            &track->channel->sends[j];
          *send = *clone_send;
          if (clone_send->is_empty)
            continue;

          reconnect_send (send);
        }

      /* reconnect any custom connections */
      int max_size = 20;
      int num_ports = 0;
      Port ** ports =
        calloc ((size_t) max_size, sizeof (Port *));
      track_append_all_ports (
        self->tls->tracks[i],
        &ports, &num_ports, true, &max_size,
        true);
      for (int j = 0; j < num_ports; j++)
        {
          Port * port = ports[j];
          Port * prj_port =
            port_find_from_identifier (&port->id);

          /*char port_label[600];*/
          /*port_get_full_designation (*/
            /*port, port_label);*/
          /*g_message (*/
            /*"port: %s %d %d", port_label,*/
            /*port->num_srcs, port->num_dests);*/

          for (int k = 0; k < port->num_srcs; k++)
            {
              Port * src_port =
                port_find_from_identifier (
                  &port->src_ids[k]);
              port_connect (
                src_port, prj_port,
                port->src_locked[k]);
              int src_idx =
                port_get_src_index (
                  prj_port, src_port);
              prj_port->src_multipliers[src_idx] =
                port->src_multipliers[k];
              prj_port->src_enabled[src_idx] =
                port->src_enabled[k];
              int dest_idx =
                port_get_dest_index (
                  src_port, prj_port);
              src_port->multipliers[dest_idx] =
                port->src_multipliers[k];
              src_port->dest_enabled[dest_idx] =
                port->src_enabled[k];
            }
          for (int k = 0; k < port->num_dests; k++)
            {
              Port * dest_port =
                port_find_from_identifier (
                  &port->dest_ids[k]);
              port_connect (
                prj_port, dest_port,
                port->dest_locked[k]);
              int dest_idx =
                port_get_dest_index (
                  prj_port, dest_port);
              prj_port->multipliers[dest_idx] =
                port->multipliers[k];
              prj_port->dest_enabled[dest_idx] =
                port->dest_enabled[k];
              int src_idx =
                port_get_src_index (
                  dest_port, prj_port);
              dest_port->src_multipliers[src_idx] =
                port->multipliers[k];
              dest_port->src_enabled[src_idx] =
                port->dest_enabled[k];
            }
        }
      free (ports);
    }

  /* re-connect any source sends */
  for (int i = 0; i < self->num_src_sends; i++)
    {
      ChannelSend * clone_send = &self->src_sends[i];

      /* get the original send and connect it */
      Track * orig_track =
        TRACKLIST->tracks[clone_send->track_pos];
      ChannelSend * send =
        &orig_track->channel->sends[
          clone_send->slot];

      reconnect_send (send);
    }

  EVENTS_PUSH (ET_TRACKS_ADDED, NULL);

  /* recalculate graph */
  router_recalc_graph (ROUTER, F_NOT_SOFT);

  return 0;
}

char *
delete_tracks_action_stringize (
  DeleteTracksAction * self)
{
  if (self->tls->num_tracks == 1)
    return g_strdup (_("Delete Track"));
  else
    return g_strdup_printf (
      _("Delete %d Tracks"),
      self->tls->num_tracks);
}

void
delete_tracks_action_free (
  DeleteTracksAction * self)
{
  object_free_w_func_and_null (
    tracklist_selections_free, self->tls);

  object_zero_and_free (self);
}

