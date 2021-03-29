/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/mixer_selections_action.h"
#include "audio/channel.h"
#include "audio/modulator_track.h"
#include "audio/router.h"
#include "audio/track.h"
#include "gui/backend/mixer_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
mixer_selections_action_init_loaded (
  MixerSelectionsAction * self)
{
  if (self->ms_before)
    {
      mixer_selections_init_loaded (
        self->ms_before, false);
    }
  if (self->deleted_ms)
    {
      mixer_selections_init_loaded (
        self->deleted_ms, false);
    }

  for (int i = 0; i < self->num_ats; i++)
    {
      automation_track_init_loaded (self->ats[i]);
    }
  for (int i = 0; i < self->num_deleted_ats; i++)
    {
      automation_track_init_loaded (
        self->deleted_ats[i]);
    }
}

/**
 * Clone automation tracks.
 *
 * @param deleted Use deleted_ats.
 * @param start_slot Slot in \ref ms to start
 *   processing.
 */
static void
clone_ats (
  MixerSelectionsAction * self,
  MixerSelections *       ms,
  bool                    deleted,
  int                     start_slot)
{
  Track * track =
    TRACKLIST->tracks[ms->track_pos];
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  for (int j = 0;
       j < ms->num_slots; j++)
    {
      int slot = ms->slots[j];
      for (int i = 0; i < atl->num_ats; i++)
        {
          AutomationTrack * at = atl->ats[i];
          if (at->port_id.owner_type !=
                PORT_OWNER_TYPE_PLUGIN ||
              at->port_id.plugin_id.slot != slot ||
              at->port_id.plugin_id.slot_type !=
                ms->type)
            continue;

          if (deleted)
            {
              self->deleted_ats[
                self->num_deleted_ats++] =
                  automation_track_clone (at);
            }
          else
            {
              self->ats[self->num_ats++] =
                automation_track_clone (at);
            }
        }
    }
}

/**
 * Create a new action.
 *
 * @param ms The mixer selections before the action
 *   is performed.
 * @param slot_type Target slot type.
 * @param to_track_pos Target track position.
 * @param to_slot Target slot.
 * @param setting The plugin setting, if creating
 *   plugins.
 * @param num_plugins The number of plugins to create,
 *   if creating plugins.
 */
UndoableAction *
mixer_selections_action_new (
  MixerSelections *         ms,
  MixerSelectionsActionType type,
  PluginSlotType            slot_type,
  int                       to_track_pos,
  int                       to_slot,
  PluginSetting *           setting,
  int                       num_plugins)
{
  MixerSelectionsAction * self =
    object_new (MixerSelectionsAction);
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_MIXER_SELECTIONS;

  self->type = type;
  self->slot_type = slot_type;
  self->to_slot = to_slot;
  self->to_track_pos = to_track_pos;
  if (to_track_pos == -1)
    {
      self->new_channel = true;
    }
  if (setting)
    {
      self->setting =
        plugin_setting_clone (setting, true);
    }
  self->num_plugins = num_plugins;

  if (ms)
    {
      self->ms_before =
        mixer_selections_clone (
          ms, ms == MIXER_SELECTIONS);
      if (!self->ms_before)
        {
          g_warning (
            "failed to clone mixer selections");
          return NULL;
        }
      g_warn_if_fail (
        ms->slots[0] == self->ms_before->slots[0]);

      /* clone the automation tracks */
      clone_ats (self, self->ms_before, false, 0);
    }

  return ua;
}

static void
copy_at_regions (
  AutomationTrack * dest,
  AutomationTrack * src)
{
  dest->regions_size = (size_t) src->num_regions;
  dest->num_regions = src->num_regions;
  dest->regions =
    g_realloc (
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
 * Revers automation events from before deletion.
 *
 * @param deleted Whether to use deleted_ats.
 */
static void
revert_automation (
  MixerSelectionsAction * self,
  Track *                 track,
  MixerSelections *       ms,
  int                     slot,
  bool                    deleted)
{
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  for (int k = 0;
       k < ms->num_slots; k++)
    {
      for (int j = 0;
           j <
             (deleted ?
                self->num_deleted_ats :
                self->num_ats);
           j++)
        {
          AutomationTrack * cloned_at =
            (deleted ?
               self->deleted_ats[j] :
               self->ats[j]);

          if (cloned_at->port_id.plugin_id.slot !=
                ms->slots[k] ||
              cloned_at->
                port_id.plugin_id.slot_type !=
                  ms->type)
            {
              continue;
            }

          /* find corresponding automation
           * track in track and copy
           * regions */
          AutomationTrack * actual_at =
            automation_tracklist_get_plugin_at (
              atl, ms->type, slot,
              cloned_at->port_id.label);

          copy_at_regions (
            actual_at, cloned_at);
        }
    }
}

/**
 * Save an existing plugin about to be replaced.
 */
static void
save_existing_plugin (
  MixerSelectionsAction * self,
  MixerSelections *       tmp_ms,
  PluginSlotType          slot_type,
  Track *                 from_tr,
  int                     from_slot,
  Track *                 to_tr,
  int                     to_slot)
{
  Plugin * existing_pl =
    track_get_plugin_at_slot (
      to_tr, slot_type, to_slot);
  if (existing_pl &&
      (from_tr != to_tr ||
       from_slot != to_slot))
    {
      mixer_selections_add_slot (
        tmp_ms, to_tr,
        slot_type, to_slot, F_CLONE);
      clone_ats (
        self, tmp_ms, true,
        tmp_ms->num_slots - 1);
    }
}

static void
revert_deleted_plugin (
  MixerSelectionsAction * self,
  Track *                 to_tr,
  int                     to_slot)
{
  if (!self->deleted_ms)
    {
      g_debug (
        "No deleted plugin to revert at %s#%d",
        to_tr->name, to_slot);
      return;
    }

  if (self->deleted_ms->type ==
        PLUGIN_SLOT_MODULATOR)
    {
      /* modulators are never replaced */
      return;
    }

  for (int j = 0;
       j < self->deleted_ms->num_slots; j++)
    {
      int slot_to_revert =
        self->deleted_ms->slots[j];
      if (slot_to_revert != to_slot)
        {
          continue;
        }

      g_message (
        "reverting plugin %s in slot %d",
        self->deleted_ms->plugins[j]->setting->descr->name,
        slot_to_revert);

      /* note: this also instantiates the
       * plugin */
      Plugin * new_pl =
        plugin_clone (
          self->deleted_ms->plugins[j],
          F_NOT_PROJECT);

      /* add to channel */
      track_add_plugin (
        to_tr, self->deleted_ms->type,
        slot_to_revert, new_pl,
        F_REPLACING, F_NOT_MOVING_PLUGIN,
        F_GEN_AUTOMATABLES,
        F_NO_RECALC_GRAPH,
        F_NO_PUBLISH_EVENTS);

      /* bring back automation */
      revert_automation (
        self, to_tr, self->deleted_ms,
        slot_to_revert, true);

      /* activate and set visibility */
      plugin_activate (new_pl, F_ACTIVATE);

      /* show if was visible before */
      if (ZRYTHM_HAVE_UI &&
          self->deleted_ms->plugins[j]->
            visible)
        {
          new_pl->visible = true;
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED,
            new_pl);
        }
    }
}

static int
do_or_undo_create_or_delete (
  MixerSelectionsAction * self,
  bool                    _do,
  bool                    create)
{
  Track * track =
    TRACKLIST->tracks[
      create ?
        self->to_track_pos :
        self->ms_before->track_pos];
  g_return_val_if_fail (track, -1);
  Channel * ch = track->channel;
  MixerSelections * own_ms = self->ms_before;
  PluginSlotType slot_type =
    create ? self->slot_type : own_ms->type;
  int loop_times =
    create ? self->num_plugins : own_ms->num_slots;
  bool delete = !create;

  /* if creating plugins (create do or delete undo) */
  if ((create && _do) || (delete && !_do))
    {
      /* clear deleted caches */
      for (int j = self->num_deleted_ats - 1;
           j >= 0; j--)
        {
          AutomationTrack * at =
            self->deleted_ats[j];
          object_free_w_func_and_null (
            automation_track_free, at);
          self->num_deleted_ats--;
        }
      object_free_w_func_and_null (
        mixer_selections_free, self->deleted_ms);
      self->deleted_ms = mixer_selections_new ();

      for (int i = 0; i < loop_times; i++)
        {
          int slot =
            create ?
              (self->to_slot + i) :
              own_ms->plugins[i]->id.slot;

          /* create new plugin */
          Plugin * pl = NULL;
          if (create)
            {
              pl =
                plugin_new_from_setting (
                  self->setting, self->to_track_pos,
                  slot_type, slot);
              g_return_val_if_fail (
                IS_PLUGIN_AND_NONNULL (pl), -1);

              /* instantiate so that ports are
               * created */
              int ret =
                plugin_instantiate (
                  pl, F_NOT_PROJECT, NULL);
              g_return_val_if_fail (!ret, -1);
            }
          else if (delete)
            {
              /* note: this also instantiates the
               * plugin */
              pl =
                plugin_clone (
                  own_ms->plugins[i], false);
            }

          /* validate */
          g_return_val_if_fail (pl, -1);
          if (delete)
            {
              g_return_val_if_fail (
                slot == own_ms->slots[i], -1);
            }

          /* set track */
          plugin_set_track_pos (pl, track->pos);

          /* save any plugin about to be deleted */
          save_existing_plugin (
            self, self->deleted_ms, slot_type,
            NULL, -1,
            slot_type == PLUGIN_SLOT_MODULATOR ?
              P_MODULATOR_TRACK : track,
            slot);

          /* add to destination */
          track_insert_plugin (
            track, pl, slot_type, slot,
            F_NOT_REPLACING, F_NOT_MOVING_PLUGIN,
            F_NO_CONFIRM, F_GEN_AUTOMATABLES,
            F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);

          /* copy automation from before deletion */
          if (delete)
            {
              revert_automation (
                self, track, own_ms, slot, false);
            }

          /* select the plugin */
          mixer_selections_add_slot (
            MIXER_SELECTIONS, track, slot_type,
            pl->id.slot, F_NO_CLONE);

          /* set visibility */
          if (create)
            {
              /* set visible from settings */
              pl->visible =
                ZRYTHM_HAVE_UI &&
                g_settings_get_boolean (
                  S_P_PLUGINS_UIS,
                  "open-on-instantiate");
            }
          else if (delete)
            {
              /* set visible if plugin was visible
               * before deletion */
              pl->visible =
                ZRYTHM_HAVE_UI &&
                  own_ms->plugins[i]->visible;
            }
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, pl);

          /* activate */
          int ret = plugin_activate (pl, F_ACTIVATE);
          g_return_val_if_fail (!ret, -1);
        }

      /* restore port connections */
      if (delete)
        {
          for (int i = 0; i < loop_times; i++)
            {
              Plugin * pl = own_ms->plugins[i];
              g_message (
                "restoring custom connections "
                "for plugin '%s'",
                pl->setting->descr->name);
              size_t max_size = 20;
              int num_ports = 0;
              Port ** ports =
                object_new_n (
                  max_size, Port *);
              plugin_append_ports (
                pl, &ports, &max_size, F_DYNAMIC,
                &num_ports);
              for (int j = 0; j < num_ports; j++)
                {
                  /*g_warn_if_reached ();*/
                  Port * port = ports[j];
                  Port * prj_port =
                    port_find_from_identifier (
                      &port->id);
                  port_restore_from_non_project (
                    prj_port, port);
                }
              free (ports);
            }
        }

      track_validate (track);

      EVENTS_PUSH (ET_PLUGINS_ADDED, track);
    }
  /* else if deleting plugins (create undo or delete
   * do) */
  else
    {
      for (int i = 0; i < loop_times; i++)
        {
          int slot =
            create ?
              (self->to_slot + i) :
              own_ms->plugins[i]->id.slot;

          /* if doing deletion, rememnber port
           * metadata */
          if (_do)
            {
              Plugin * own_pl = own_ms->plugins[i];
              Plugin * prj_pl =
                track_get_plugin_at_slot (
                  track, slot_type, slot);

              /* remember any custom connections */
              g_message (
                "remembering custom connections "
                "for plugin '%s'",
                own_pl->setting->descr->name);
              size_t max_size = 20;
              int num_ports = 0;
              Port ** ports =
                object_new_n (max_size, Port *);
              plugin_append_ports (
                prj_pl, &ports, &max_size,
                F_DYNAMIC, &num_ports);
              max_size = 20;
              int num_own_ports = 0;
              Port ** own_ports =
                object_new_n (max_size, Port *);
              plugin_append_ports (
                own_pl, &own_ports, &max_size,
                F_DYNAMIC, &num_own_ports);
              for (int j = 0; j < num_ports; j++)
                {
                  Port * prj_port = ports[j];

                  Port * own_port = NULL;
                  for (int k = 0;
                       k < num_own_ports; k++)
                    {
                      Port * cur_own_port =
                        own_ports[k];
                      if (port_identifier_is_equal (
                            &cur_own_port->id,
                            &prj_port->id))
                        {
                          own_port =
                            cur_own_port;
                          break;
                        }
                    }
                  g_return_val_if_fail (
                    own_port, -1);

                  port_copy_metadata_from_project (
                    own_port, prj_port);
                }
              free (ports);
              free (own_ports);
            }

          /* remove the plugin at given slot */
          track_remove_plugin (
            track, slot_type, slot,
            F_NOT_REPLACING, F_NOT_MOVING_PLUGIN,
            F_DELETING_PLUGIN, F_NOT_DELETING_TRACK,
            F_NO_RECALC_GRAPH);

          /* if there was a plugin at the slot
           * before, bring it back */
          revert_deleted_plugin (
            self, track, slot);
        }

      EVENTS_PUSH (ET_PLUGINS_REMOVED, NULL);
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  if (ch)
    {
      EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, ch);
    }

  return 0;
}

static void
copy_automation_from_track1_to_track2 (
  Track *           from_track,
  Track *           to_track,
  PluginSlotType    slot_type,
  int               from_slot,
  int               to_slot)
{
  AutomationTracklist * prev_atl =
    track_get_automation_tracklist (from_track);
  for (int j = 0; j < prev_atl->num_ats; j++)
    {
      /* get the previous at */
      AutomationTrack * prev_at = prev_atl->ats[j];
      if (prev_at->num_regions == 0 ||
          prev_at->port_id.owner_type !=
            PORT_OWNER_TYPE_PLUGIN ||
          prev_at->port_id.plugin_id.slot !=
            from_slot ||
          prev_at->port_id.plugin_id.slot_type !=
            slot_type)
        {
          continue;
        }

      /* find the corresponding at in the new
       * track */
      AutomationTracklist * atl =
        track_get_automation_tracklist (to_track);
      for (int k = 0; k < atl->num_ats; k++)
        {
          AutomationTrack * at = atl->ats[k];

          if (at->port_id.owner_type !=
                PORT_OWNER_TYPE_PLUGIN ||
              at->port_id.plugin_id.slot !=
                to_slot ||
              at->port_id.plugin_id.slot_type !=
                slot_type ||
              at->port_id.port_index !=
                prev_at->port_id.port_index)
            {
              continue;
            }

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
                to_track, new_region, at, -1, 0, 0);
            }
          break;
        }
    }
}

static int
do_or_undo_move_or_copy (
  MixerSelectionsAction * self,
  bool                    _do,
  bool                    copy)
{
  MixerSelections * own_ms = self->ms_before;
  PluginSlotType slot_type = self->slot_type;
  Track * from_tr =
    mixer_selections_get_track (own_ms);
  g_return_val_if_fail (from_tr, -1);
  bool move = !copy;

  if (_do)
    {
      Track * to_tr = NULL;

      if (self->new_channel)
        {
          /* get the own plugin */
          Plugin * own_pl = own_ms->plugins[0];

          /* add the plugin to a new track */
          char * str =
            g_strdup_printf (
              "%s (Copy)",
              own_pl->setting->descr->name);
          to_tr =
            track_new (
              TRACK_TYPE_AUDIO_BUS,
              TRACKLIST->num_tracks, str,
              F_WITH_LANE);
          g_free (str);
          g_return_val_if_fail (to_tr, -1);

          /* add the track to the tracklist */
          tracklist_append_track (
            TRACKLIST, to_tr, F_NO_PUBLISH_EVENTS,
            F_NO_RECALC_GRAPH);

          /* remember to track pos */
          self->to_track_pos = to_tr->pos;
        }
      /* else if not new track/channel */
      else
        {
          to_tr =
            TRACKLIST->tracks[self->to_track_pos];
        }

      Channel * to_ch = to_tr->channel;
      g_return_val_if_fail (IS_CHANNEL (to_ch), -1);

      mixer_selections_clear (
        MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

      /* sort own selections */
      mixer_selections_sort (own_ms, F_ASCENDING);

      bool move_downwards_same_track =
        to_tr == from_tr &&
        own_ms->num_slots > 0 &&
        self->to_slot > own_ms->plugins[0]->id.slot;
#define FOREACH_SLOT \
  for (int i = \
         move_downwards_same_track ? \
           own_ms->num_slots - 1 : 0; \
       move_downwards_same_track ? \
         (i >= 0) : (i < own_ms->num_slots); \
       move_downwards_same_track ? i-- : i++)

      /* clear deleted caches */
      for (int j = self->num_deleted_ats - 1;
           j >= 0; j--)
        {
          AutomationTrack * at = self->deleted_ats[j];
          object_free_w_func_and_null (
            automation_track_free, at);
          self->num_deleted_ats--;
        }
      object_free_w_func_and_null (
        mixer_selections_free, self->deleted_ms);
      self->deleted_ms = mixer_selections_new ();

      FOREACH_SLOT
        {
          /* get/create the actual plugin */
          int from_slot =
            own_ms->plugins[i]->id.slot;
          Plugin * pl = NULL;
          if (move)
            {
              pl =
                track_get_plugin_at_slot (
                  from_tr, own_ms->type, from_slot);
              g_warn_if_fail (
                pl &&
                pl->id.track_pos == from_tr->pos);
            }
          else
            {
              pl =
                plugin_clone (
                  own_ms->plugins[i], false);
            }
          g_return_val_if_fail (IS_PLUGIN (pl), -1);

          int to_slot = self->to_slot + i;

          /* save any plugin about to be deleted */
          save_existing_plugin (
            self, self->deleted_ms, own_ms->type,
            from_tr, from_slot, to_tr, to_slot);

          /* move or copy the plugin */
          if (move)
            {
              g_message (
                "moving plugin from %d to %d",
                from_slot, to_slot);

              if (from_tr != to_tr ||
                  from_slot != to_slot)
                {
                  plugin_move (
                    pl, to_tr, self->slot_type,
                    to_slot,
                    F_NO_PUBLISH_EVENTS);
                }
            }
          else if (copy)
            {
              g_message (
                "moving plugin from %d to %d",
                from_slot, to_slot);

              track_insert_plugin (
                to_tr, pl, slot_type, to_slot,
                F_NOT_REPLACING, F_NOT_MOVING_PLUGIN,
                F_NO_CONFIRM, F_GEN_AUTOMATABLES,
                F_NO_RECALC_GRAPH,
                F_NO_PUBLISH_EVENTS);

              g_return_val_if_fail (
                pl->num_in_ports ==
                  own_ms->plugins[i]->num_in_ports,
                -1);
            }

          /* copy automation regions from original
           * plugin */
          if (copy)
            {
              copy_automation_from_track1_to_track2 (
                from_tr, to_tr,
                slot_type, own_ms->slots[i],
                to_slot);
            }

          /* select it */
          mixer_selections_add_slot (
            MIXER_SELECTIONS, to_tr,
            slot_type, to_slot, F_NO_CLONE);

          /* if new plugin (copy), activate it and
           * set visibility */
          if (copy)
            {
              g_return_val_if_fail (
                plugin_activate (pl, F_ACTIVATE)
                  == 0,
                -1);

              /* show if was visible before */
              if (ZRYTHM_HAVE_UI &&
                  own_ms->plugins[i]->visible)
                {
                  pl->visible = true;
                  EVENTS_PUSH (
                    ET_PLUGIN_VISIBILITY_CHANGED,
                    pl);
                }
            }
        }

#undef FOREACH_SLOT

      track_validate (to_tr);

      if (self->new_channel)
        {
          EVENTS_PUSH (ET_TRACKS_ADDED, NULL);
        }

      EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, to_ch);
    }
  /* else if undoing (deleting/moving plugins back) */
  else
    {
      Track * to_tr =
        TRACKLIST->tracks[self->to_track_pos];
      Channel * to_ch = to_tr->channel;
      g_return_val_if_fail (IS_TRACK (to_tr), -1);

      /* clear selections to readd each original
       * plugin */
      mixer_selections_clear (
        MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

      /* sort own selections */
      mixer_selections_sort (own_ms, F_ASCENDING);

      bool move_downwards_same_track =
        to_tr == from_tr&&
        own_ms->num_slots > 0 &&
        self->to_slot < own_ms->plugins[0]->id.slot;
      for (int i =
             move_downwards_same_track ?
               own_ms->num_slots - 1 : 0;
           move_downwards_same_track ?
             (i >= 0) : (i < own_ms->num_slots);
           move_downwards_same_track ? i-- : i++)
        {
          /* get the actual plugin */
          int to_slot = self->to_slot + i;
          Plugin * pl =
            track_get_plugin_at_slot (
              to_tr, slot_type, to_slot);
          g_return_val_if_fail (IS_PLUGIN (pl), -1);

          /* original slot */
          int from_slot =
            own_ms->plugins[i]->id.slot;

          /* if moving plugins back */
          if (move)
            {
              /* move plugin to its original slot */
              g_message (
                "moving plugin back from %d to %d",
                to_slot, from_slot);
              if (from_tr!= to_tr ||
                  from_slot != to_slot)
                {
                  Plugin * existing_pl =
                    track_get_plugin_at_slot (
                      from_tr, own_ms->type,
                      from_slot);
                  g_warn_if_fail (!existing_pl);
                  plugin_move (
                    pl, from_tr, own_ms->type,
                    from_slot, F_NO_PUBLISH_EVENTS);
                }
            }
          else if (copy)
            {
              track_remove_plugin (
                to_tr, slot_type, to_slot,
                F_NOT_REPLACING,
                F_NOT_MOVING_PLUGIN,
                F_DELETING_PLUGIN,
                F_NOT_DELETING_TRACK,
                F_NO_RECALC_GRAPH);
              pl = NULL;
            }

          /* if there was a plugin at the slot
           * before, bring it back */
          revert_deleted_plugin (
            self, to_tr, to_slot);

          if (copy)
            {
              pl =
                track_get_plugin_at_slot (
                  from_tr, slot_type, from_slot);
            }

          /* add orig plugin to mixer selections */
          g_warn_if_fail (IS_PLUGIN (pl));
          mixer_selections_add_slot (
            MIXER_SELECTIONS, from_tr,
            own_ms->type, pl->id.slot, F_NO_CLONE);
        }

      /* if a new track was created delete it */
      if (self->new_channel)
        {
          tracklist_remove_track (
            TRACKLIST, to_tr, F_REMOVE_PL, F_FREE,
            F_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
        }

      track_validate (from_tr);

      EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, to_ch);
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  return 0;
}

static int
do_or_undo (
  MixerSelectionsAction * self,
  bool                    _do)
{
  switch (self->type)
    {
    case MIXER_SELECTIONS_ACTION_CREATE:
      return
        do_or_undo_create_or_delete (
          self, _do, true);
    case MIXER_SELECTIONS_ACTION_DELETE:
      return
        do_or_undo_create_or_delete (
          self, _do, false);
    case MIXER_SELECTIONS_ACTION_MOVE:
      return
        do_or_undo_move_or_copy (
          self, _do, false);
      break;
    case MIXER_SELECTIONS_ACTION_COPY:
      return
        do_or_undo_move_or_copy (
          self, _do, true);
      break;
    default:
      g_warn_if_reached ();
    }
  return -1;
}

int
mixer_selections_action_do (
  MixerSelectionsAction * self)
{
  return do_or_undo (self, true);
}

int
mixer_selections_action_undo (
  MixerSelectionsAction * self)
{
  return do_or_undo (self, false);
}

char *
mixer_selections_action_stringize (
  MixerSelectionsAction * self)
{
  switch (self->type)
    {
    case MIXER_SELECTIONS_ACTION_CREATE:
      if (self->num_plugins == 1)
        {
          return g_strdup_printf (
            _("Create %s"),
            self->setting->descr->name);
        }
      else
        {
          return g_strdup_printf (
            _("Create %d %ss"),
            self->num_plugins,
            self->setting->descr->name);
        }
    case MIXER_SELECTIONS_ACTION_DELETE:
      if (self->ms_before->num_slots == 1)
        {
          return g_strdup_printf (
            _("Delete Plugin"));
        }
      else
        {
          return g_strdup_printf (
            _("Delete %d Plugins"),
            self->ms_before->num_slots);
        }
    case MIXER_SELECTIONS_ACTION_MOVE:
      if (self->ms_before->num_slots == 1)
        {
          return g_strdup_printf (
            _("Move %s"),
            self->ms_before->
              plugins[0]->setting->descr->name);
        }
      else
        {
          return g_strdup_printf (
            _("Move %d Plugins"),
            self->ms_before->num_slots);
        }
    case MIXER_SELECTIONS_ACTION_COPY:
      if (self->ms_before->num_slots == 1)
        {
          return g_strdup_printf (
            _("Copy %s"),
            self->ms_before->
              plugins[0]->setting->descr->name);
        }
      else
        {
          return g_strdup_printf (
            _("Copy %d Plugins"),
            self->ms_before->num_slots);
        }
    default:
      g_warn_if_reached ();
    }

  return NULL;
}

void
mixer_selections_action_free (
  MixerSelectionsAction * self)
{
  object_free_w_func_and_null (
    mixer_selections_free, self->ms_before);

  for (int j = 0; j < self->num_ats; j++)
    {
      AutomationTrack * at = self->ats[j];
      object_free_w_func_and_null (
        automation_track_free, at);
    }
  for (int j = 0; j < self->num_deleted_ats; j++)
    {
      AutomationTrack * at = self->deleted_ats[j];
      object_free_w_func_and_null (
        automation_track_free, at);
    }

  object_free_w_func_and_null (
    plugin_setting_free, self->setting);

  object_zero_and_free (self);
}
