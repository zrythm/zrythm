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
    self->to_track_pos = to_tr->pos;
  else
    self->is_new_channel = 1;

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
  Channel * from_ch =
    TRACKLIST->tracks[self->from_track_pos]->channel;
  g_return_val_if_fail (from_ch, -1);
  Channel * to_ch = NULL;
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
          plugin_descriptor_is_instrument (
            clone_pl->descr) ?
          TRACK_TYPE_INSTRUMENT :
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

      to_ch = new_track->channel;
    }
  else
    {
      to_ch =
        TRACKLIST->tracks[self->to_track_pos]->
          channel;
      g_return_val_if_fail (to_ch, -1);
    }

  /*int highest_slot =*/
    /*mixer_selections_get_highest_slot (self->ms);*/

  /* clear selections to readd each plugin moved */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

  /*int diff;*/
  int to_slot;
  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* get the plugin */
      switch (self->ms->type)
        {
        case PLUGIN_SLOT_MIDI_FX:
          pl =
            from_ch->midi_fx[
              self->ms->plugins[i]->id.slot];
          break;
        case PLUGIN_SLOT_INSTRUMENT:
          pl = from_ch->instrument;
          break;
        case PLUGIN_SLOT_INSERT:
          pl =
            from_ch->inserts[
              self->ms->plugins[i]->id.slot];
          break;
        }
      g_warn_if_fail (
        pl &&
        pl->id.track_pos == from_ch->track_pos);

      /* move and select plugin to to_slot + diff */
      to_slot = self->to_slot + i;
      plugin_move (
        pl, to_ch, self->slot_type, to_slot);

      mixer_selections_add_slot (
        MIXER_SELECTIONS, to_ch, self->slot_type,
        to_slot);
    }

  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, to_ch);

  return 0;
}

int
move_plugins_action_undo (
  MovePluginsAction * self)
{
  /* get original channel */
  Channel * ch =
    TRACKLIST->tracks[
      self->from_track_pos]->channel;
  g_return_val_if_fail (ch, -1);

  /* get the channel the plugins were moved to */
  Channel * current_ch =
    TRACKLIST->tracks[self->to_track_pos]->channel;
  g_return_val_if_fail (current_ch, -1);

  /* clear selections to readd each plugin moved */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* get the actual plugin */
      Plugin * pl = NULL;
      switch (self->slot_type)
        {
        case PLUGIN_SLOT_MIDI_FX:
          pl =
            current_ch->midi_fx[self->to_slot + i];
          break;
        case PLUGIN_SLOT_INSTRUMENT:
          pl = current_ch->instrument;
          break;
        case PLUGIN_SLOT_INSERT:
          pl =
            current_ch->inserts[self->to_slot + i];
          break;
        }
      g_return_val_if_fail (pl, -1);

      /* move plugin to its original slot */
      plugin_move (
        pl, ch, self->ms->type,
        self->ms->plugins[i]->id.slot);

      /* add to mixer selections */
      mixer_selections_add_slot (
        MIXER_SELECTIONS, ch, self->ms->type,
        pl->id.slot);
    }

  if (self->is_new_channel)
    {
      tracklist_remove_track (
        TRACKLIST, current_ch->track, F_REMOVE_PL,
        F_FREE, F_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);
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
