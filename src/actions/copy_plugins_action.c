/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
  Track *           tr,
  int               slot)
{
  CopyPluginsAction * self =
    calloc (1, sizeof (
      CopyPluginsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UA_COPY_PLUGINS;

  self->ms = mixer_selections_clone (ms);
  self->slot = slot;

  if (tr)
    self->track_pos = tr->pos;
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

      /* add the plugin to a new track */
      char * str =
        g_strdup_printf (
          "%s (Copy)",
          orig_pl->descr->name);
      track =
        track_new (
          plugin_descriptor_is_instrument (
            orig_pl->descr) ?
          TRACK_TYPE_INSTRUMENT :
          TRACK_TYPE_AUDIO_BUS,
          TRACKLIST->num_tracks, str,
          F_WITH_LANE);
      g_free (str);
      g_return_val_if_fail (track, -1);

      /* create a track and add it to
       * tracklist */
      tracklist_insert_track (
        TRACKLIST,
        track,
        TRACKLIST->num_tracks,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      EVENTS_PUSH (ET_TRACKS_ADDED, NULL);

      ch = track->channel;
    }
  else
    {
      /* else add the plugin to the given
       * channel */
      track = TRACKLIST->tracks[self->track_pos];
      ch = track->channel;
    }
  g_return_val_if_fail (ch, -1);


  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* clone the clone */
      pl = plugin_clone (self->ms->plugins[i]);

      /* add it to the channel */
      channel_add_plugin (
        ch, self->slot + i, pl, 1, 1,
        F_NO_RECALC_GRAPH);

      /* show it if necessary */
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
copy_plugins_action_undo (
  CopyPluginsAction * self)
{
  Track * tr =
    TRACKLIST->tracks[self->track_pos];
  g_warn_if_fail (tr);

  /* if a new channel was created just delete the
   * channel */
  if (self->is_new_channel)
    {

      tracklist_remove_track (
        TRACKLIST,
        tr,
        F_REMOVE_PL,
        F_FREE,
        F_PUBLISH_EVENTS,
        F_RECALC_GRAPH);

      return 0;
    }

  /* no new channel, delete each plugin */
  Channel * ch = tr->channel;
  g_warn_if_fail (ch);

  for (int i = 0; i < self->ms->num_slots; i++)
    {
      channel_remove_plugin (
        ch, self->slot + i, 1, 0,
        F_NO_RECALC_GRAPH);
    }

  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED,
               ch);

  mixer_recalc_graph (MIXER);

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
