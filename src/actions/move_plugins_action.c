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
  self->to_ch_id = to_ch->id;
  self->ms = mixer_selections_clone (ms);

  return ua;
}

int
move_plugins_action_do (
	MovePluginsAction * self)
{
  Plugin * pl;
  Channel * ch =
    project_get_channel (self->to_ch_id);
  g_return_val_if_fail (ch, -1);

  int highest_slot =
    mixer_selections_get_highest_slot (self->ms);
  int diff;
  for (int i = 0; i < self->ms->num_slots; i++)
    {
      pl =
        project_get_plugin (
          self->ms->plugins[i]->id);

      /* get difference in slots */
      diff = self->ms->slots[i] - highest_slot;
      g_return_val_if_fail (diff > -1, -1);

      mixer_move_plugin (
        MIXER, pl, ch, self->to_slot + diff);
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
    project_get_channel (
      self->ms->plugins[0]->channel_id);
  g_return_val_if_fail (ch, -1);

  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* get the actual plugin from the project
       * matching the clone's ID
       * Remember that the clone should not be in
       * the project */
      pl =
        project_get_plugin (
          self->ms->plugins[i]->id);

      /* move plugin to its original slot */
      mixer_move_plugin (
        MIXER, pl, ch, pl->slot);
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
