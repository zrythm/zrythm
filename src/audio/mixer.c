/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file
 * Mixer implementation. */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "audio/audio_region.h"
#include "audio/audio_track.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/instrument_track.h"
#include "audio/mixer.h"
#include "audio/region.h"
#include "audio/routing.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/audio.h"
#include "utils/arrays.h"
#include "utils/dialogs.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

/**
 * Recalculates the process acyclic directed graph.
 */
void
mixer_recalc_graph (
  Mixer * mixer)
{
  Router * router = &mixer->router;
  if (!router->graph)
    {
      router_init (router);
      router->graph = graph_new (router);
      graph_setup (router->graph, 1, 1);
      graph_start (router->graph);
      return;
    }

  zix_sem_wait (&router->graph_access);
  graph_setup (router->graph, 1, 1);
  zix_sem_post (&router->graph_access);
}

void
mixer_init_loaded ()
{
}

/**
 * Loads plugins from state files. Used when loading projects.
 */
void
mixer_load_plugins (
  Mixer * mixer)
{
  Track * track;
  Channel * ch;
  Plugin * pl;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      ch = track->channel;
      if (!ch)
        continue;

      for (int j = 0; j < STRIP_SIZE; j++)
        {
          pl = ch->plugins[j];
          if (!pl)
            continue;

          plugin_instantiate (pl);
        }
    }
}

/**
 * Moves the given plugin to the given slot in
 * the given channel.
 *
 * If a plugin already exists, it deletes it and
 * replaces it.
 */
void
mixer_move_plugin (
  Mixer *   self,
  Plugin *  pl,
  Channel * ch,
  int       slot)
{
  g_return_if_fail (pl);
  g_return_if_fail (ch);

  /* confirm if another plugin exists */
  if (ch->plugins[slot])
    {
      GtkDialog * dialog =
        dialogs_get_overwrite_plugin_dialog (
          GTK_WINDOW (MAIN_WINDOW));
      int result =
        gtk_dialog_run (dialog);
      gtk_widget_destroy (GTK_WIDGET (dialog));

      /* do nothing if not accepted */
      if (result != GTK_RESPONSE_ACCEPT)
        return;
    }

  int prev_slot = pl->id.slot;
  Channel * prev_ch = plugin_get_channel (pl);

  /* move plugin's automation from src to dest */
  plugin_move_automation (
    pl, prev_ch, ch, slot);

  /* remove plugin from its channel */
  channel_remove_plugin (
    prev_ch, prev_slot, 0, 0, F_NO_RECALC_GRAPH);

  /* add plugin to its new channel */
  channel_add_plugin (
    ch, slot, pl, 0, 0, F_RECALC_GRAPH);

  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, prev_ch);
  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, ch);
}
