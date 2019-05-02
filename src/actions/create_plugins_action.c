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

#include "actions/create_plugins_action.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"

#include <glib/gi18n.h>

UndoableAction *
create_plugins_action_new (
  PluginDescriptor * descr,
  Channel * ch,
  int       slot,
  int       num_plugins)
{
	CreatePluginsAction * self =
    calloc (1, sizeof (
    	CreatePluginsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_CREATE_PLUGINS;

  self->slot = slot;
  self->ch_id = ch->id;

  for (int i = 0; i < num_plugins; i++)
    {
      self->plugins[i] =
        plugin_new_from_descr (
          descr, F_NO_ADD_TO_PROJ);
    }
  self->num_plugins = num_plugins;

  return ua;
}

int
create_plugins_action_do (
	CreatePluginsAction * self)
{
  Plugin * pl;
  Channel * ch =
    project_get_channel (self->ch_id);
  g_return_val_if_fail (ch, -1);

  for (int i = 0; i < self->num_plugins; i++)
    {
      /* clone the clone */
      pl = plugin_clone (self->plugins[i]);
      g_return_val_if_fail (pl, -1);

      /* add to project to get unique ID */
      project_add_plugin (pl);

      /* add to channel */
      channel_add_plugin (
        ch, self->slot + i, pl, 1, 1, 1);

      if (g_settings_get_int (
            S_PREFERENCES,
            "open-plugin-uis-on-instantiate"))
        {
          pl->visible = 1;
          EVENTS_PUSH (ET_PLUGIN_VISIBILITY_CHANGED,
                       pl);
        }

      /* remember the ID */
      self->plugins[i]->id = pl->id;
    }

  return 0;
}

/**
 * Deletes the plugins.
 */
int
create_plugins_action_undo (
	CreatePluginsAction * self)
{
  Channel * ch =
    project_get_channel (self->ch_id);
  g_return_val_if_fail (ch, -1);

  for (int i = 0; i < self->num_plugins; i++)
    {
      /* remove the plugin */
      channel_remove_plugin (
        ch, self->slot + i, 1, 0);
    }

  return 0;
}

char *
create_plugins_action_stringize (
	CreatePluginsAction * self)
{
  if (self->num_plugins == 1)
    return g_strdup_printf (
      _("Create %s"),
      self->plugins[0]->descr->name);
  else
    return g_strdup_printf (
      _("Create %d %ss"),
      self->num_plugins,
      self->plugins[0]->descr->name);
}

void
create_plugins_action_free (
	CreatePluginsAction * self)
{
  free (self);
}

