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

#include "actions/delete_plugins_action.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"

#include <glib/gi18n.h>

UndoableAction *
delete_plugins_action_new (
  MixerSelections * ms,
  Track *           tr,
  int               slot)
{
	DeletePluginsAction * self =
    calloc (1, sizeof (
    	DeletePluginsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_DELETE_PLUGINS;

  self->tr_id = tr->id;
  self->slot = slot;
  self->ms = mixer_selections_clone (ms);

  return ua;
}

int
delete_plugins_action_do (
	DeletePluginsAction * self)
{
  Plugin * pl;
  Channel * ch =
    project_get_track (self->tr_id)->channel;
  g_return_val_if_fail (ch, -1);

  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* get actual plugin */
      pl =
        project_get_plugin (
          self->ms->plugins[i]->id);

      /* remove it */
      channel_remove_plugin (
        ch, pl->slot, 1, 0);
    }

  return 0;
}

/**
 * Deletes the plugin.
 */
int
delete_plugins_action_undo (
	DeletePluginsAction * self)
{
  Plugin * pl, * orig_pl;
  Channel * ch =
    project_get_track (self->tr_id)->channel;
  g_return_val_if_fail (ch, -1);

  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* get clone */
      orig_pl = self->ms->plugins[i];

      /* clone the clone */
      pl = plugin_clone (orig_pl);

      /* add to project to get unique ID */
      project_add_plugin (pl);

      /* add plugin to channel */
      /* FIXME automation track info is completely
       * lost*/
      channel_add_plugin (
        ch, orig_pl->slot, pl,
        F_NO_CONFIRM,
        F_GEN_AUTOMATABLES,
        F_NO_RECALC_GRAPH);

      /* remember the ID */
      orig_pl->id = pl->id;
    }

  mixer_recalc_graph (MIXER);

  EVENTS_PUSH (ET_PLUGINS_ADDED, NULL);

  return 0;
}

char *
delete_plugins_action_stringize (
	DeletePluginsAction * self)
{
  if (self->ms->num_slots == 1)
    return g_strdup_printf (
      _("Delete Plugin"));
  else
    return g_strdup_printf (
      _("Delete %d Plugins"),
      self->ms->num_slots);
}

void
delete_plugins_action_free (
	DeletePluginsAction * self)
{
  mixer_selections_free (self->ms);

  free (self);
}
