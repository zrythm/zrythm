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

#include "actions/move_plugin_action.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "plugins/plugin.h"
#include "project.h"

UndoableAction *
move_plugin_action_new (
  Plugin *  pl,
  Channel * to_ch,
  int       to_slot)
{
	MovePluginAction * self =
    calloc (1, sizeof (
    	MovePluginAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_MOVE_PLUGIN;
  self->from_slot =
    channel_get_plugin_index (pl->channel, pl);
  self->from_ch_id = pl->channel_id;
  self->to_slot = to_slot;
  self->to_ch_id = to_ch->id;
  self->pl_id = pl->id;
  return ua;
}

int
move_plugin_action_do (
	MovePluginAction * self)
{
  Plugin * pl = project_get_plugin (self->pl_id);
  Channel * ch =
    project_get_channel (self->to_ch_id);
  g_warn_if_fail (pl);
  g_warn_if_fail (ch);

  mixer_move_plugin (
    MIXER, pl, ch, self->to_slot);

  return 0;
}

int
move_plugin_action_undo (
	MovePluginAction * self)
{
  Plugin * pl = project_get_plugin (self->pl_id);
  Channel * ch =
    project_get_channel (self->from_ch_id);
  g_warn_if_fail (pl);
  g_warn_if_fail (ch);

  mixer_move_plugin (
    MIXER, pl, ch, self->from_slot);

  return 0;
}

void
move_plugin_action_free (
	MovePluginAction * self)
{
  free (self);
}
