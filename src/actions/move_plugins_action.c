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

#include "actions/move_plugins_action.h"
#include "audio/channel.h"
#include "audio/router.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/mixer_selections.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
move_plugins_action_init_loaded (
  MovePluginsAction * self)
{
  mixer_selections_init_loaded (self->ms, false);
}

/**
 * Create a new action.
 *
 * @param slot_type Slot type to copy to.
 */
UndoableAction *
move_plugins_action_new (
  MixerSelections * ms,
  PluginSlotType   slot_type,
  Track *    to_tr,
  int        to_slot)
{
  MovePluginsAction * self =
    calloc (1, sizeof (
      MovePluginsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UA_MOVE_PLUGINS;

  self->slot_type = slot_type;
  self->to_slot = to_slot;
  self->from_track_pos = ms->track_pos;
  if (to_tr)
    {
      self->to_track_pos = to_tr->pos;
    }
  else
    {
      self->is_new_channel = true;
    }

  self->ms =
    mixer_selections_clone (
      ms, ms == MIXER_SELECTIONS);
  g_warn_if_fail (
    ms->slots[0] == self->ms->slots[0]);

  return ua;
}

int
move_plugins_action_do (
  MovePluginsAction * self)
{
  Plugin * pl = NULL;
  Track * from_tr =
    TRACKLIST->tracks[self->from_track_pos];
  Channel * from_ch = from_tr->channel;
  g_return_val_if_fail (from_ch, -1);
  Channel * to_ch = NULL;
  Track * to_tr = NULL;
  if (self->is_new_channel)
    {
      /* get the clone */
      Plugin * clone_pl = self->ms->plugins[0];

      /* add the plugin to a new track */
      char * str =
        g_strdup_printf (
          "%s (Copy)",
          clone_pl->descr->name);
      Track * new_track =
        track_new (
          TRACK_TYPE_AUDIO_BUS,
          TRACKLIST->num_tracks, str,
          F_WITH_LANE);
      g_free (str);
      g_return_val_if_fail (new_track, -1);

      /* create a track and add it to
       * tracklist */
      tracklist_insert_track (
        TRACKLIST, new_track,
        TRACKLIST->num_tracks,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      EVENTS_PUSH (ET_TRACKS_ADDED, NULL);

      /* remember track pos */
      self->to_track_pos = new_track->pos;

      to_tr = new_track;
      to_ch = new_track->channel;
    }
  else
    {
      to_tr =
        TRACKLIST->tracks[self->to_track_pos];
      to_ch = to_tr->channel;
      g_return_val_if_fail (to_tr, -1);
    }

  /* clear selections to readd each plugin moved */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

  /* sort own selections */
  mixer_selections_sort (self->ms, F_ASCENDING);

  bool move_downwards_same_track =
    to_tr == from_tr &&
    self->ms->num_slots > 0 &&
    self->to_slot > self->ms->plugins[0]->id.slot;
  for (int i =
         move_downwards_same_track ?
           self->ms->num_slots - 1 : 0;
       move_downwards_same_track ?
         (i >= 0) : (i < self->ms->num_slots);
       move_downwards_same_track ? i-- : i++)
    {
      /* get the plugin */
      int from_slot = self->ms->plugins[i]->id.slot;
      pl =
        track_get_plugin_at_slot (
          from_tr, self->ms->type, from_slot);
      g_warn_if_fail (
        pl && pl->id.track_pos == from_tr->pos);

      /* move and select plugin to to_slot + diff */
      int to_slot = self->to_slot + i;
      g_message (
        "moving plugin from %d to %d",
        from_slot, to_slot);
      if (from_tr != to_tr ||
          from_slot != to_slot)
        {
          plugin_move (
            pl, to_tr, self->slot_type, to_slot,
            F_NO_PUBLISH_EVENTS);
        }

      mixer_selections_add_slot (
        MIXER_SELECTIONS, to_tr, self->slot_type,
        to_slot);
    }

  track_verify_identifiers (to_tr);

  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, to_ch);

  return 0;
}

int
move_plugins_action_undo (
  MovePluginsAction * self)
{
  /* get original channel */
  Track * track =
    TRACKLIST->tracks[self->from_track_pos];
  Channel * ch = track->channel;
  g_return_val_if_fail (track, -1);

  /* get the channel the plugins were moved to */
  Track * current_tr =
    TRACKLIST->tracks[self->to_track_pos];
  Channel * current_ch = current_tr->channel;
  g_return_val_if_fail (current_ch, -1);

  /* clear selections to readd each plugin moved */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

  /* sort own selections */
  mixer_selections_sort (self->ms, F_ASCENDING);

  bool move_downwards_same_track =
    current_tr == track &&
    self->ms->num_slots > 0 &&
    self->to_slot < self->ms->plugins[0]->id.slot;
  for (int i =
         move_downwards_same_track ?
           self->ms->num_slots - 1 : 0;
       move_downwards_same_track ?
         (i >= 0) : (i < self->ms->num_slots);
       move_downwards_same_track ? i-- : i++)
    {
      /* get the actual plugin */
      int from_slot = self->to_slot + i;
      Plugin * pl =
        track_get_plugin_at_slot (
          current_tr, self->slot_type,
          from_slot);
      g_return_val_if_fail (pl, -1);

      /* move plugin to its original slot */
      int to_slot = self->ms->plugins[i]->id.slot;
      g_message (
        "moving plugin from %d to %d",
        from_slot, to_slot);
      if (track != current_tr ||
          from_slot != to_slot)
        {
          plugin_move (
            pl, track, self->ms->type, to_slot,
            F_NO_PUBLISH_EVENTS);
        }

      /* add to mixer selections */
      mixer_selections_add_slot (
        MIXER_SELECTIONS, track, self->ms->type,
        pl->id.slot);
    }

  if (self->is_new_channel)
    {
      tracklist_remove_track (
        TRACKLIST, current_ch->track, F_REMOVE_PL,
        F_FREE, F_PUBLISH_EVENTS,
        F_RECALC_GRAPH);
    }

  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, ch);

  return 0;
}

char *
move_plugins_action_stringize (
  MovePluginsAction * self)
{
  if (self->ms->num_slots == 1)
    return g_strdup_printf (
      _("Move %s"),
      self->ms->plugins[0]->descr->name);
  else
    return g_strdup_printf (
      _("Move %d Plugins"),
      self->ms->num_slots);
}

void
move_plugins_action_free (
  MovePluginsAction * self)
{
  free (self);
}
