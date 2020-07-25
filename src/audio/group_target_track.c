/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "audio/channel.h"
#include "audio/group_target_track.h"
#include "audio/router.h"
#include "audio/track.h"
#include "utils/arrays.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

void
group_target_track_init_loaded (
  Track * self)
{
  self->children_size = self->num_children;
}

void
group_target_track_init (
  Track * self)
{
  self->children_size = 1;
  self->children =
    calloc (
      (size_t) self->children_size, sizeof (int));
}

/**
 * Updates the output of the child channel (where the
 * Channel routes to).
 */
static void
update_child_output (
  Channel * ch,
  Track *   output,
  bool      recalc_graph,
  bool      pub_events)
{
  if (ch->has_output)
    {
      Track * track =
        channel_get_output_track (ch);
      /* disconnect Channel's output from the
       * current
       * output channel */
      switch (track->in_signal_type)
        {
        case TYPE_AUDIO:
          port_disconnect (
            ch->stereo_out->l,
            track->processor->stereo_in->l);
          port_disconnect (
            ch->stereo_out->r,
            track->processor->stereo_in->r);
          break;
        case TYPE_EVENT:
          port_disconnect (
            ch->midi_out,
            track->processor->midi_in);
          break;
        default:
          break;
        }
    }

  if (output)
    {
      /* connect Channel's output to the given
       * output */
      switch (output->in_signal_type)
        {
        case TYPE_AUDIO:
          port_connect (
            ch->stereo_out->l,
            output->processor->stereo_in->l, 1);
          port_connect (
            ch->stereo_out->r,
            output->processor->stereo_in->r, 1);
          break;
        case TYPE_EVENT:
          port_connect (
            ch->midi_out,
            output->processor->midi_in, 1);
          break;
        default:
          break;
        }
      ch->has_output = true;
      ch->output_pos = output->pos;
    }
  else
    {
      ch->has_output = 0;
      ch->output_pos = -1;
    }

  if (recalc_graph)
    {
      router_recalc_graph (ROUTER, F_NOT_SOFT);
    }

  if (pub_events)
    {
      EVENTS_PUSH (ET_CHANNEL_OUTPUT_CHANGED, ch);
    }
}

/**
 * Removes a child track from the list of children.
 */
void
group_target_track_remove_child (
  Track * self,
  int     child_pos,
  bool    disconnect,
  bool    recalc_graph,
  bool    pub_events)
{
  g_return_if_fail (child_pos != self->pos);

  Track * child = TRACKLIST->tracks[child_pos];
  g_message (
    "removing %s (%d) from %s (%d) - disconnect %d",
    self->name, self->pos, child->name, child->pos,
    disconnect);

  if (disconnect)
    {
      update_child_output (
        child->channel, NULL, recalc_graph,
        pub_events);
    }
  array_delete_primitive (
    self->children, self->num_children, child_pos);
}

/**
 * Remove all known children.
 *
 * @param disconnect Also route the children to "None".
 */
void
group_target_track_remove_all_children (
  Track * self,
  bool    disconnect,
  bool    recalc_graph,
  bool    pub_events)
{
  for (int i = self->num_children - 1; i >= 0; i--)
    {
      group_target_track_remove_child (
        self, self->children[i], disconnect,
        recalc_graph, pub_events);
    }
}

/**
 * Adds a child track to the list of children.
 *
 * @param connect Connect the child to the group track.
 */
void
group_target_track_add_child (
  Track * self,
  int     child_pos,
  bool    connect,
  bool    recalc_graph,
  bool    pub_events)
{
  g_return_if_fail (
    IS_TRACK (self) && self->children);
  if (connect)
    {
      update_child_output (
        TRACKLIST->tracks[child_pos]->channel, self,
        recalc_graph, pub_events);
    }

  array_append (
    self->children, self->num_children, child_pos);
}

void
group_target_track_add_children (
  Track * self,
  int *   children,
  int     num_children,
  bool    connect,
  bool    recalc_graph,
  bool    pub_events)
{
  for (int i = 0; i < num_children; i++)
    {
      group_target_track_add_child (
        self, children[i], connect, recalc_graph,
        pub_events);
    }
}
