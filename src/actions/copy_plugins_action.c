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
#include "audio/router.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
copy_plugins_action_init_loaded (
  CopyPluginsAction * self)
{
  mixer_selections_init_loaded (self->ms, false);
}

/**
 * Create a new CopyPluginsAction.
 *
 * @param slot Starting slot to copy plugins to.
 * @param tr Track to copy to.
 * @param slot_type Slot type to copy to.
 */
UndoableAction *
copy_plugins_action_new (
  MixerSelections * ms,
  PluginSlotType   slot_type,
  Track *           tr,
  int               slot)
{
  CopyPluginsAction * self =
    calloc (1, sizeof (
      CopyPluginsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UA_COPY_PLUGINS;

  self->slot_type = slot_type;
  self->ms =
    mixer_selections_clone (
      ms, ms == MIXER_SELECTIONS);
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

      /* remember track pos */
      self->track_pos = track->pos;

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

  mixer_selections_clear (MIXER_SELECTIONS, 0);

  /* get previous track */
  Track * prev_track =
    mixer_selections_get_track (self->ms);
  g_return_val_if_fail (prev_track, -1);
  AutomationTracklist * prev_atl =
    track_get_automation_tracklist (prev_track);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);

  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* clone the clone */
      pl =
        plugin_clone (self->ms->plugins[i], false);

      /* add it to the channel */
      int new_slot = self->slot + i;
      channel_add_plugin (
        ch, self->slot_type, new_slot, pl, 1, 1,
        F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);

      /* copy the automation regions from the
       * original plugin */
      for (int j = 0; j < prev_atl->num_ats; j++)
        {
          /* get the previous at */
          AutomationTrack * prev_at =
            prev_atl->ats[j];
          if (prev_at->num_regions == 0 ||
              prev_at->port_id.owner_type !=
                PORT_OWNER_TYPE_PLUGIN ||
              prev_at->port_id.plugin_id.slot !=
                self->ms->slots[i] ||
              prev_at->port_id.plugin_id.slot_type !=
                self->ms->type)
            continue;

          /* find the corresponding at in the new
           * track */
          for (int k = 0; k < atl->num_ats; k++)
            {
              AutomationTrack * at = atl->ats[k];

              if (at->port_id.owner_type !=
                    PORT_OWNER_TYPE_PLUGIN ||
                  at->port_id.plugin_id.slot !=
                    new_slot ||
                  at->port_id.plugin_id.slot_type !=
                    self->slot_type ||
                  at->port_id.port_index !=
                    prev_at->port_id.port_index)
                continue;

              /* copy the automation regions */
              for (int l = 0;
                   l < prev_at->num_regions; l++)
                {
                  ZRegion * prev_region =
                    prev_at->regions[l];
                  ZRegion * new_region =
                    (ZRegion *)
                    arranger_object_clone (
                      (ArrangerObject *)
                      prev_region,
                      ARRANGER_OBJECT_CLONE_COPY_MAIN);
                  track_add_region (
                    track, new_region, at, -1, 0, 0);
                }
              break;
            }
        }

      /* select it */
      mixer_selections_add_slot (
        MIXER_SELECTIONS, ch, self->slot_type,
        new_slot);

      /* show it if it was visible before */
      if (ZRYTHM_HAVE_UI &&
          self->ms->plugins[i]->visible)
        {
          pl->visible = 1;
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, pl);
        }
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, ch);

  return 0;
}

/**
 * Deletes the plugins.
 */
int
copy_plugins_action_undo (
  CopyPluginsAction * self)
{
  Track * tr = TRACKLIST->tracks[self->track_pos];
  Channel * ch = tr->channel;
  g_warn_if_fail (tr);

  /* if a new channel was created just delete the
   * channel */
  if (self->is_new_channel)
    {
      tracklist_remove_track (
        TRACKLIST, tr, F_REMOVE_PL, F_FREE,
        F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
    }
  /* no new channel, delete each plugin */
  else
    {
      g_warn_if_fail (ch);

      for (int i = 0; i < self->ms->num_slots; i++)
        {
          channel_remove_plugin (
            ch, self->slot_type,
            self->slot + i, 1, 0,
            F_NO_RECALC_GRAPH);
        }
    }

  /* reselect the previous plugins */
  Channel * prev_ch =
    TRACKLIST->tracks[self->ms->track_pos]->
      channel;
  for (int i = 0; i < self->ms->num_slots; i++)
    {
      mixer_selections_add_slot (
        MIXER_SELECTIONS, prev_ch,
        self->ms->type,
        self->ms->slots[i]);
      /*g_message ("readding slot %d",*/
        /*self->ms->slots[i]);*/
    }

  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, ch);

  router_recalc_graph (ROUTER, F_NOT_SOFT);

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
