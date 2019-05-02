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

#include "actions/copy_plugins_action.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"

#include <glib/gi18n.h>

UndoableAction *
copy_plugins_action_new (
  MixerSelections * ms,
  Channel *         ch,
  int               slot)
{
	CopyPluginsAction * self =
    calloc (1, sizeof (
    	CopyPluginsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_COPY_PLUGINS;

  self->ms = mixer_selections_clone (ms);
  self->slot = slot;

  if (ch)
    self->ch_id = ch->id;
  else
    self->is_new_channel = 1;

  return ua;
}

int
copy_plugins_action_do (
	CopyPluginsAction * self)
{
  Plugin * orig_pl, * pl;
  Channel * ch;
  Track * track = NULL;

  /* get the channel and track */
  if (self->is_new_channel)
    {
      /* get the clone */
      orig_pl = self->ms->plugins[0];

      /* add the plugin to a new channel */
      ChannelType ct;
      if (plugin_is_instrument (orig_pl))
        ct = CT_MIDI;
      else
        ct = CT_BUS;

      char * str =
        g_strdup_printf (
          "%s (Copy)",
          orig_pl->descr->name);
      ch =
        channel_new (
          ct, str, 1);
      g_warn_if_fail (ch);

      /* also create a track and add it to
       * tracklist */
      track = track_new (ch, str);
      g_free (str);
      tracklist_insert_track (
        TRACKLIST,
        track,
        TRACKLIST->num_tracks,
        F_NO_PUBLISH_EVENTS);

      channel_connect (ch);
    }
  else
    {
      /* else add the plugin to the given
       * channel */
      ch =
        project_get_channel (self->ch_id);
    }
  g_return_val_if_fail (ch, -1);


  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* get the clone */
      orig_pl = self->ms->plugins[i];

      /* clone the clone */
      pl = plugin_clone (orig_pl);

      /* add to project to get unique ID */
      project_add_plugin (pl);

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

      /* remember the ID for when undoing */
      orig_pl->id = pl->id;
    }

  return 0;
}

/**
 * Deletes the plugins.
 */
int
copy_plugins_action_undo (
	CopyPluginsAction * self)
{
  Plugin * pl;

  /* if a new channel was created just delete the
   * channel */
  if (self->is_new_channel)
    {
      pl =
        project_get_plugin (
          self->ms->plugins[0]->id);
      g_return_val_if_fail (pl, -1);

      channel_disconnect (
        pl->channel);
      mixer_remove_channel (
        MIXER,
        pl->channel,
        F_FREE,
        F_PUBLISH_EVENTS);
      tracklist_remove_track (
        TRACKLIST,
        pl->channel->track,
        F_FREE,
        F_PUBLISH_EVENTS);

      return 0;
    }

  /* no new channel, delete each plugin */
  Channel * ch =
    project_get_channel (self->ch_id);
  g_warn_if_fail (ch);

  for (int i = 0; i < self->ms->num_slots; i++)
    {
      channel_remove_plugin (
        ch, self->slot + i, 1, 0);
    }

  return 0;
}

char *
copy_plugins_action_stringize (
	CopyPluginsAction * self)
{
  if (self->ms->num_slots == 1)
    return g_strdup_printf (
      _("Copy %s"),
      self->ms->plugins[0]->descr->name);
  else
    return g_strdup_printf (
      _("Copy %d Plugins"),
      self->ms->num_slots);
}

void
copy_plugins_action_free (
	CopyPluginsAction * self)
{
  mixer_selections_free (self->ms);

  free (self);
}
