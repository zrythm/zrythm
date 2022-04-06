// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdlib.h>

#include "audio/channel.h"
#include "audio/group_target_track.h"
#include "audio/router.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "zrythm.h"
#include "zrythm_app.h"

void
group_target_track_init_loaded (Track * self)
{
  self->children_size = (size_t) self->num_children;
  if (self->num_children == 0)
    {
      self->children_size = 1;
      self->children = calloc (
        (size_t) self->children_size, sizeof (int));
    }
}

void
group_target_track_init (Track * self)
{
  self->children_size = 1;
  self->children = calloc (
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
  g_return_if_fail (ch);

  if (ch->has_output)
    {
      Track * track = channel_get_output_track (ch);
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
            ch->midi_out, track->processor->midi_in);
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
            output->processor->stereo_in->l,
            F_LOCKED);
          port_connect (
            ch->stereo_out->r,
            output->processor->stereo_in->r,
            F_LOCKED);
          break;
        case TYPE_EVENT:
          port_connect (
            ch->midi_out,
            output->processor->midi_in, F_LOCKED);
          break;
        default:
          break;
        }
      ch->has_output = true;
      ch->output_name_hash =
        track_get_name_hash (output);
    }
  else
    {
      ch->has_output = 0;
      ch->output_name_hash = 0;
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

static bool
contains_child (
  Track *      self,
  unsigned int child_name_hash)
{
  for (int i = 0; i < self->num_children; i++)
    {
      if (self->children[i] == child_name_hash)
        {
          return true;
        }
    }

  return false;
}

/**
 * Removes a child track from the list of children.
 */
void
group_target_track_remove_child (
  Track *      self,
  unsigned int child_name_hash,
  bool         disconnect,
  bool         recalc_graph,
  bool         pub_events)
{
  g_return_if_fail (
    child_name_hash != track_get_name_hash (self));
  g_return_if_fail (
    contains_child (self, child_name_hash));

  Tracklist * tracklist = track_get_tracklist (self);

  Track * child = tracklist_find_track_by_name_hash (
    tracklist, child_name_hash);
  g_return_if_fail (IS_TRACK_AND_NONNULL (child));
  g_message (
    "removing '%s' from '%s' - disconnect? %d",
    child->name, self->name, disconnect);

  if (disconnect)
    {
      update_child_output (
        child->channel, NULL, recalc_graph,
        pub_events);
    }
  array_delete_primitive (
    self->children, self->num_children,
    child_name_hash);

  g_message (
    "removed '%s' from direct out '%s' - "
    "num children: %d",
    child->name, self->name, self->num_children);
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

bool
group_target_track_validate (Track * self)
{
  for (int i = 0; i < self->num_children; i++)
    {
      Track * track =
        tracklist_find_track_by_name_hash (
          TRACKLIST, self->children[i]);
      g_return_val_if_fail (
        IS_TRACK_AND_NONNULL (track), false);
      Track * out_track =
        channel_get_output_track (track->channel);
      g_return_val_if_fail (
        self == out_track, false);
    }

  return true;
}

/**
 * Adds a child track to the list of children.
 *
 * @param connect Connect the child to the group track.
 */
void
group_target_track_add_child (
  Track *      self,
  unsigned int child_name_hash,
  bool         connect,
  bool         recalc_graph,
  bool         pub_events)
{
  g_return_if_fail (
    IS_TRACK (self) && self->children);

  g_debug (
    "adding child track with name hash %u to "
    "group %s",
    child_name_hash, self->name);

  if (connect)
    {
      Tracklist * tracklist =
        track_get_tracklist (self);
      Track * out_track =
        tracklist_find_track_by_name_hash (
          tracklist, child_name_hash);
      g_return_if_fail (
        IS_TRACK_AND_NONNULL (out_track)
        && out_track->channel);
      update_child_output (
        out_track->channel, self, recalc_graph,
        pub_events);
    }

  array_double_size_if_full (
    self->children, self->num_children,
    self->children_size, unsigned int);
  array_append (
    self->children, self->num_children,
    child_name_hash);
}

void
group_target_track_add_children (
  Track *        self,
  unsigned int * children,
  int            num_children,
  bool           connect,
  bool           recalc_graph,
  bool           pub_events)
{
  for (int i = 0; i < num_children; i++)
    {
      group_target_track_add_child (
        self, children[i], connect, recalc_graph,
        pub_events);
    }
}

/**
 * Returns the index of the child matching the
 * given hash.
 */
int
group_target_track_find_child (
  Track *      self,
  unsigned int track_name_hash)
{
  for (int i = 0; i < self->num_children; i++)
    {
      if (track_name_hash == self->children[i])
        {
          return i;
        }
    }

  return -1;
}
