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

#include "actions/move_plugins_action.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "gui/backend/mixer_selections.h"
#include "plugins/plugin.h"
#include "project.h"

#include <glib/gi18n.h>

UndoableAction *
move_plugins_action_new (
  MixerSelections * ms,
  Channel *  to_ch,
  int        to_slot)
{
	MovePluginsAction * self =
    calloc (1, sizeof (
    	MovePluginsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_MOVE_PLUGINS;

  self->to_slot = to_slot;
  self->from_track_pos =
    ms->plugins[0]->channel->track->pos;
  if (to_ch)
    self->to_track_pos = to_ch->track->pos;
  else
    self->is_new_channel = 1;

  self->ms = mixer_selections_clone (ms);

  return ua;
}

int
move_plugins_action_do (
	MovePluginsAction * self)
{
  Plugin * pl;
  Channel * from_ch =
    TRACKLIST->tracks[self->from_track_pos]->channel;
  Channel * to_ch =
    TRACKLIST->tracks[self->to_track_pos]->channel;
  g_return_val_if_fail (from_ch, -1);
  g_return_val_if_fail (to_ch, -1);

  /*int highest_slot =*/
    /*mixer_selections_get_highest_slot (self->ms);*/

  /* clear selections to readd each plugin moved */
  mixer_selections_clear (MIXER_SELECTIONS);

  /*int diff;*/
  int to_slot;
  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* get the plugin */
      pl =
        from_ch->plugins[self->ms->plugins[i]->slot];

      /* get difference in slots */
      /*diff = self->ms->slots[i] - highest_slot;*/
      /*g_return_val_if_fail (diff > -1, -1);*/

      /* move and select plugin to to_slot + diff */
      to_slot = self->to_slot + i;
      mixer_move_plugin (
        MIXER, pl, to_ch, to_slot);

      mixer_selections_add_slot (
        MIXER_SELECTIONS, to_ch, to_slot);
    }

  return 0;
}

int
move_plugins_action_undo (
	MovePluginsAction * self)
{
  Plugin * pl;

  /* get original channel */
  Channel * ch =
    TRACKLIST->tracks[
      self->from_track_pos]->channel;
  g_return_val_if_fail (ch, -1);

  /* clear selections to readd each plugin moved */
  mixer_selections_clear (MIXER_SELECTIONS);

  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* get the actual plugin */
      pl = ch->plugins[self->to_slot + i];

      /* move plugin to its original slot */
      mixer_move_plugin (
        MIXER, pl, ch, self->ms->plugins[i]->slot);

      /* add to mixer selections */
      mixer_selections_add_slot (
        MIXER_SELECTIONS, ch, pl->slot);
    }

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
