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

#include "actions/delete_plugins_action.h"
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
delete_plugins_action_init_loaded (
  DeletePluginsAction * self)
{
  mixer_selections_init_loaded (self->ms, false);

  for (int i = 0; i < self->num_ats; i++)
    {
      automation_track_init_loaded (self->ats[i]);
    }
}

UndoableAction *
delete_plugins_action_new (
  MixerSelections * ms)
{
  DeletePluginsAction * self =
    calloc (1, sizeof (
      DeletePluginsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UA_DELETE_PLUGINS;

  self->ms =
    mixer_selections_clone (
      ms, ms == MIXER_SELECTIONS);

  /* clone the automation tracks */
  Track * track =
    TRACKLIST->tracks[self->ms->track_pos];
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  for (int j = 0; j < self->ms->num_slots; j++)
    {
      int slot = self->ms->slots[j];
      for (int i = 0; i < atl->num_ats; i++)
        {
          AutomationTrack * at = atl->ats[i];
          if (at->port_id.owner_type !=
                PORT_OWNER_TYPE_PLUGIN ||
              at->port_id.plugin_id.slot != slot ||
              at->port_id.plugin_id.slot_type !=
                self->ms->type)
            continue;

          self->ats[self->num_ats++] =
            automation_track_clone (at);
        }
    }

  return ua;
}

int
delete_plugins_action_do (
  DeletePluginsAction * self)
{
  Channel * ch =
    TRACKLIST->tracks[self->ms->track_pos]->channel;
  g_return_val_if_fail (ch, -1);

  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* remove the plugin at given slot */
      channel_remove_plugin (
        ch, self->ms->type,
        self->ms->plugins[i]->id.slot, 1, 0,
        F_NO_RECALC_GRAPH);
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  return 0;
}

static void
copy_regions (
  AutomationTrack * dest,
  AutomationTrack * src)
{
  dest->regions_size = (size_t) src->num_regions;
  dest->num_regions = src->num_regions;
  dest->regions =
    realloc (
      dest->regions,
      dest->regions_size * sizeof (ZRegion *));

  for (int j = 0; j < src->num_regions; j++)
    {
      ZRegion * src_region = src->regions[j];
      dest->regions[j] =
        (ZRegion *)
        arranger_object_clone (
          (ArrangerObject *) src_region,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);
      region_set_automation_track (
        dest->regions[j], dest);
    }
}

/**
 * Deletes the plugin.
 */
int
delete_plugins_action_undo (
  DeletePluginsAction * self)
{
  Plugin * pl;
  Track * track =
    TRACKLIST->tracks[self->ms->track_pos];
  Channel * ch = track->channel;
  g_return_val_if_fail (ch, -1);

  for (int i = 0; i < self->ms->num_slots; i++)
    {
      int slot = self->ms->slots[i];

      /* clone the clone */
      pl =
        plugin_clone (self->ms->plugins[i], false);

      g_return_val_if_fail (
        self->ms->plugins[i]->id.slot == slot, -1);

      /* add plugin to channel at original slot */
      channel_add_plugin (
        ch, self->ms->type, slot, pl,
        F_NO_CONFIRM,
        F_GEN_AUTOMATABLES,
        F_NO_RECALC_GRAPH,
        F_NO_PUBLISH_EVENTS);

      /* copy automation */
      AutomationTracklist * atl =
        track_get_automation_tracklist (track);
      for (int k = 0; k < self->ms->num_slots; k++)
        {
          for (int j = 0; j < self->num_ats; j++)
            {
              AutomationTrack * cloned_at =
                self->ats[j];

              if (cloned_at->port_id.plugin_id.slot !=
                    self->ms->slots[k] ||
                  cloned_at->port_id.plugin_id.slot_type !=
                    self->ms->type)
                continue;

              /* find corresponding automation
               * track in track and copy regions */
              AutomationTrack * actual_at =
                automation_tracklist_get_plugin_at (
                  atl, self->ms->type, slot,
                  cloned_at->port_id.label);

              copy_regions (actual_at, cloned_at);
            }
        }

      /* instantiate and activate it */
      plugin_instantiate (pl, true, NULL);
      plugin_activate (pl, F_ACTIVATE);

      /* select the plugin */
      mixer_selections_add_slot (
        MIXER_SELECTIONS, ch, self->ms->type,
        pl->id.slot);

      /* show it if it was visible before */
      if (ZRYTHM_HAVE_UI &&
          self->ms->plugins[i]->visible)
        {
      g_message ("visible plugin at %d",
        pl->id.slot);
          pl->visible = 1;
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, pl);
        }
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  EVENTS_PUSH (ET_PLUGINS_ADDED, ch);
  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, ch);

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

  for (int j = 0; j < self->num_ats; j++)
    {
      AutomationTrack * at = self->ats[j];
      automation_track_free (at);
    }

  free (self);
}
