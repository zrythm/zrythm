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

#include "actions/create_plugins_action.h"
#include "audio/channel.h"
#include "audio/modulator_track.h"
#include "audio/router.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

UndoableAction *
create_plugins_action_new (
  const PluginDescriptor * descr,
  PluginSlotType  slot_type,
  int       track_pos,
  int       slot,
  int       num_plugins)
{
  CreatePluginsAction * self =
    calloc (1, sizeof (
      CreatePluginsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UA_CREATE_PLUGINS;

  self->slot_type = slot_type;
  self->slot = slot;
  self->track_pos = track_pos;
  plugin_descriptor_copy (descr, &self->descr);
  self->num_plugins = num_plugins;

  return ua;
}

int
create_plugins_action_do (
  CreatePluginsAction * self)
{
  Track * track =
    TRACKLIST->tracks[self->track_pos];
  Channel * ch = track->channel;
  g_return_val_if_fail (track, -1);

  for (int i = 0; i < self->num_plugins; i++)
    {
      /* create new plugin */
      Plugin * pl =
        plugin_new_from_descr (
          &self->descr, self->track_pos,
          self->slot + i);
      g_return_val_if_fail (pl, -1);

      /* set track */
      plugin_set_track_pos (pl, self->track_pos);

      /* instantiate */
      int ret = plugin_instantiate (pl, true, NULL);
      g_return_val_if_fail (!ret, -1);

      if (self->slot_type == PLUGIN_SLOT_MODULATOR)
        {
          modulator_track_insert_modulator (
            P_MODULATOR_TRACK, self->slot + i, pl,
            F_NOT_REPLACING,
            F_CONFIRM, F_GEN_AUTOMATABLES,
            F_NO_RECALC_GRAPH, F_PUBLISH_EVENTS);
        }
      else
        {
          /* add to channel */
          channel_add_plugin (
            ch, self->slot_type,
            self->slot + i, pl, F_CONFIRM,
            F_NOT_MOVING_PLUGIN, F_GEN_AUTOMATABLES,
            F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);
        }

      if (ZRYTHM_HAVE_UI &&
          g_settings_get_boolean (
            S_P_PLUGINS_UIS,
            "open-on-instantiate"))
        {
          pl->visible = true;
        }
      else
        {
          pl->visible = false;
        }
      EVENTS_PUSH (
        ET_PLUGIN_VISIBILITY_CHANGED, pl);

      /* activate */
      ret = plugin_activate (pl, F_ACTIVATE);
      g_return_val_if_fail (!ret, -1);
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  if (ch)
    {
      EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, ch);
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
  Track * track =
    TRACKLIST->tracks[self->track_pos];
  Channel * ch = track->channel;
  g_return_val_if_fail (track, -1);

  for (int i = 0; i < self->num_plugins; i++)
    {
      /* remove the plugin */
      if (self->slot_type == PLUGIN_SLOT_MODULATOR)
        {
          modulator_track_remove_modulator (
            track, self->slot + i,
            F_NOT_REPLACING,
            F_DELETING_PLUGIN,
            F_NOT_DELETING_TRACK,
            F_NO_RECALC_GRAPH);
        }
      else
        {
          channel_remove_plugin (
            ch, self->slot_type, self->slot + i,
            F_NOT_MOVING_PLUGIN,
            F_DELETING_PLUGIN,
            F_NOT_DELETING_CHANNEL,
            F_NO_RECALC_GRAPH);
        }

      EVENTS_PUSH (ET_PLUGINS_REMOVED, track);
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

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

