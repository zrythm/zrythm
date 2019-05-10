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
  int       track_pos,
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
  self->track_pos = track_pos;
  plugin_clone_descr (descr, &self->descr);
  self->num_plugins = num_plugins;

  for (int i = 0; i < num_plugins; i++)
    {
      self->plugins[i] =
        plugin_new_from_descr (
          &self->descr);
    }

  return ua;
}

int
create_plugins_action_do (
	CreatePluginsAction * self)
{
  Plugin * pl;
  Channel * ch =
    TRACKLIST->tracks[self->track_pos]->channel;
  g_return_val_if_fail (ch, -1);

  for (int i = 0; i < self->num_plugins; i++)
    {
      /* create new plugin */
      pl =
        plugin_new_from_descr (&self->descr);
      g_return_val_if_fail (pl, -1);

      /* set track */
      plugin_set_track (
        pl, TRACKLIST->tracks[self->track_pos]);

      /* instantiate */
      int ret = plugin_instantiate (pl);
      g_return_val_if_fail (!ret, -1);

      /* add to channel */
      channel_add_plugin (
        ch, self->slot + i, pl, 1, 1,
        F_NO_RECALC_GRAPH);

      if (g_settings_get_int (
            S_PREFERENCES,
            "open-plugin-uis-on-instantiate"))
        {
          pl->visible = 1;
          EVENTS_PUSH (ET_PLUGIN_VISIBILITY_CHANGED,
                       pl);
        }
    }

  mixer_recalc_graph (MIXER);

  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED,
               ch);

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
    TRACKLIST->tracks[self->track_pos]->channel;
  g_return_val_if_fail (ch, -1);

  for (int i = 0; i < self->num_plugins; i++)
    {
      /* remove the plugin */
      channel_remove_plugin (
        ch, self->slot + i, 1, 0,
        F_NO_RECALC_GRAPH);

      EVENTS_PUSH (ET_PLUGINS_REMOVED,
                   ch);
    }

  mixer_recalc_graph (MIXER);

  return 0;
}

char *
create_plugins_action_stringize (
	CreatePluginsAction * self)
{
  if (self->num_plugins == 1)
    return g_strdup_printf (
      _("Create %s"),
      self->descr.name);
  else
    return g_strdup_printf (
      _("Create %d %ss"),
      self->num_plugins,
      self->descr.name);
}

void
create_plugins_action_free (
	CreatePluginsAction * self)
{
  free (self);
}

