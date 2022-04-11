/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/mixer_selections_action.h"
#include "audio/channel.h"
#include "audio/modulator_track.h"
#include "audio/router.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/mixer_selections.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

typedef enum
{
  Z_ACTIONS_MIXER_SELECTIONS_ERROR_FAILED,
} ZActionsMixerSelectionsError;

#define Z_ACTIONS_MIXER_SELECTIONS_ERROR \
  z_actions_mixer_selections_error_quark ()
GQuark
z_actions_mixer_selections_error_quark (void);
G_DEFINE_QUARK (
  z-actions-mixer-selections-error-quark, z_actions_mixer_selections_error)

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
      automation_track_init_loaded (
        self->ats[i], NULL);
    }
  for (int i = 0; i < self->num_deleted_ats; i++)
    {
      automation_track_init_loaded (
        self->deleted_ats[i], NULL);
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
  Track * track = tracklist_find_track_by_name_hash (
    TRACKLIST, ms->track_name_hash);
  g_message (
    "cloning automation tracks for track %s",
    track->name);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  int count = 0;
  int regions_count = 0;
  for (int j = 0; j < ms->num_slots; j++)
    {
      int slot = ms->slots[j];
      for (int i = 0; i < atl->num_ats; i++)
        {
          AutomationTrack * at = atl->ats[i];
          if (
            at->port_id.owner_type
              != PORT_OWNER_TYPE_PLUGIN
            || at->port_id.plugin_id.slot != slot
            || at->port_id.plugin_id.slot_type
                 != ms->type)
            continue;

          if (deleted)
            {
              self->deleted_ats
                [self->num_deleted_ats++] =
                automation_track_clone (at);
            }
          else
            {
              self->ats[self->num_ats++] =
                automation_track_clone (at);
            }
          count++;
          regions_count += at->num_regions;
        }
    }
  g_message (
    "cloned %d automation tracks for track %s, "
    "total regions %d",
    count, track->name, regions_count);
}

/**
 * Create a new action.
 *
 * @param ms The mixer selections before the action
 *   is performed.
 * @param slot_type Target slot type.
 * @param to_track_name_hash Target track name hash,
 *   or 0 for new channel.
 * @param to_slot Target slot.
 * @param setting The plugin setting, if creating
 *   plugins.
 * @param num_plugins The number of plugins to create,
 *   if creating plugins.
 */
UndoableAction *
mixer_selections_action_new (
  MixerSelections *              ms,
  const PortConnectionsManager * connections_mgr,
  MixerSelectionsActionType      type,
  PluginSlotType                 slot_type,
  unsigned int    to_track_name_hash,
  int             to_slot,
  PluginSetting * setting,
  int             num_plugins,
  GError **       error)
{
  MixerSelectionsAction * self =
    object_new (MixerSelectionsAction);
  UndoableAction * ua = (UndoableAction *) self;
  undoable_action_init (ua, UA_MIXER_SELECTIONS);

  self->type = type;
  self->slot_type = slot_type;
  self->to_slot = to_slot;
  self->to_track_name_hash = to_track_name_hash;
  if (to_track_name_hash == 0)
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
      self->ms_before = mixer_selections_clone (
        ms, ms == MIXER_SELECTIONS);
      if (!self->ms_before)
        {
          g_set_error_literal (
            error, Z_ACTIONS_MIXER_SELECTIONS_ERROR,
            Z_ACTIONS_MIXER_SELECTIONS_ERROR_FAILED,
            _ ("Failed to clone mixer selections"));
          return NULL;
        }
      g_warn_if_fail (
        ms->slots[0] == self->ms_before->slots[0]);

      /* clone the automation tracks */
      clone_ats (self, self->ms_before, false, 0);
    }

  if (connections_mgr)
    self->connections_mgr_before =
      port_connections_manager_clone (
        connections_mgr);

  return ua;
}

MixerSelectionsAction *
mixer_selections_action_clone (
  const MixerSelectionsAction * src)
{
  MixerSelectionsAction * self =
    object_new (MixerSelectionsAction);

  self->parent_instance = src->parent_instance;
  self->type = src->type;
  self->slot_type = src->slot_type;
  self->to_slot = src->to_slot;
  self->to_track_name_hash =
    src->to_track_name_hash;
  self->new_channel = src->new_channel;
  self->num_plugins = src->num_plugins;
  if (src->setting)
    self->setting =
      plugin_setting_clone (src->setting, false);
  if (src->ms_before)
    self->ms_before = mixer_selections_clone (
      src->ms_before, F_NOT_PROJECT);
  if (src->deleted_ms)
    self->deleted_ms = mixer_selections_clone (
      src->deleted_ms, F_NOT_PROJECT);

  for (int i = 0; i < src->num_ats; i++)
    {
      self->ats[i] =
        automation_track_clone (src->ats[i]);
    }
  self->num_ats = src->num_ats;

  for (int i = 0; i < src->num_deleted_ats; i++)
    {
      self->deleted_ats[i] = automation_track_clone (
        src->deleted_ats[i]);
    }
  self->num_deleted_ats = src->num_deleted_ats;

  if (src->connections_mgr_before)
    self->connections_mgr_before =
      port_connections_manager_clone (
        src->connections_mgr_before);
  if (src->connections_mgr_after)
    self->connections_mgr_after =
      port_connections_manager_clone (
        src->connections_mgr_after);

  return self;
}

bool
mixer_selections_action_perform (
  MixerSelections *              ms,
  const PortConnectionsManager * connections_mgr,
  MixerSelectionsActionType      type,
  PluginSlotType                 slot_type,
  unsigned int    to_track_name_hash,
  int             to_slot,
  PluginSetting * setting,
  int             num_plugins,
  GError **       error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    mixer_selections_action_new, error, ms,
    connections_mgr, type, slot_type,
    to_track_name_hash, to_slot, setting,
    num_plugins, error);
}

static void
copy_at_regions (
  AutomationTrack * dest,
  AutomationTrack * src)
{
  dest->regions_size = (size_t) src->num_regions;
  dest->num_regions = src->num_regions;
  dest->regions = g_realloc (
    dest->regions,
    dest->regions_size * sizeof (ZRegion *));

  for (int j = 0; j < src->num_regions; j++)
    {
      ZRegion * src_region = src->regions[j];
      dest->regions[j] =
        (ZRegion *) arranger_object_clone (
          (ArrangerObject *) src_region);
      region_set_automation_track (
        dest->regions[j], dest);
    }

  if (dest->num_regions > 0)
    {
      g_message (
        "reverted %d regions for "
        "automation track %d:",
        dest->num_regions, dest->index);
      port_identifier_print (&dest->port_id);
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
  g_message (
    "reverting automation for %s#%d", track->name,
    slot);

  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  int num_ats =
    deleted ? self->num_deleted_ats : self->num_ats;
  AutomationTrack ** ats =
    deleted ? self->deleted_ats : self->ats;
  int num_reverted_ats = 0;
  int num_reverted_regions = 0;
  for (int j = 0; j < num_ats; j++)
    {
      AutomationTrack * cloned_at = ats[j];

      if (
        cloned_at->port_id.plugin_id.slot != slot
        || cloned_at->port_id.plugin_id.slot_type
             != ms->type)
        {
          continue;
        }

      /* find corresponding automation
       * track in track and copy
       * regions */
      AutomationTrack * actual_at =
        automation_tracklist_get_plugin_at (
          atl, ms->type, slot,
          cloned_at->port_id.port_index,
          cloned_at->port_id.sym);

      copy_at_regions (actual_at, cloned_at);
      num_reverted_regions +=
        actual_at->num_regions;
      num_reverted_ats++;
    }

  g_message (
    "reverted %d automation tracks and %d regions",
    num_reverted_ats, num_reverted_regions);
}

static void
reset_port_connections (
  MixerSelectionsAction * self,
  bool                    _do)
{
  if (_do && self->connections_mgr_after)
    {
      port_connections_manager_reset (
        PORT_CONNECTIONS_MGR,
        self->connections_mgr_after);
    }
  else if (!_do && self->connections_mgr_before)
    {
      port_connections_manager_reset (
        PORT_CONNECTIONS_MGR,
        self->connections_mgr_before);
    }
}

/**
 * Save an existing plugin about to be replaced
 * into @ref tmp_ms.
 */
static void
save_existing_plugin (
  MixerSelectionsAction * self,
  MixerSelections *       tmp_ms,
  Track *                 from_tr,
  PluginSlotType          from_slot_type,
  int                     from_slot,
  Track *                 to_tr,
  PluginSlotType          to_slot_type,
  int                     to_slot)
{
  Plugin * existing_pl = track_get_plugin_at_slot (
    to_tr, to_slot_type, to_slot);
  g_debug (
    "existing plugin at (%s:%s:%d => %s:%s:%d): %s",
    from_tr ? from_tr->name : "(none)",
    plugin_slot_type_to_string (from_slot_type),
    from_slot, to_tr ? to_tr->name : "(none)",
    plugin_slot_type_to_string (to_slot_type),
    to_slot,
    existing_pl
      ? existing_pl->setting->descr->name
      : "(none)");
  if (
    existing_pl
    && (from_tr != to_tr || from_slot_type != to_slot_type || from_slot != to_slot))
    {
      mixer_selections_add_slot (
        tmp_ms, to_tr, to_slot_type, to_slot,
        F_CLONE);
      clone_ats (
        self, tmp_ms, true, tmp_ms->num_slots - 1);
    }
  else
    {
      g_message (
        "skipping saving slot and cloning "
        "automation tracks - same slot");
    }
}

/**
 * @return Non-zero if error.
 */
static int
revert_deleted_plugin (
  MixerSelectionsAction * self,
  Track *                 to_tr,
  int                     to_slot,
  GError **               error)
{
  if (!self->deleted_ms)
    {
      g_debug (
        "No deleted plugin to revert at %s#%d",
        to_tr->name, to_slot);
      return 0;
    }

  g_message (
    "reverting deleted plugin at %s#%d",
    to_tr->name, to_slot);

  if (self->deleted_ms->type == PLUGIN_SLOT_MODULATOR)
    {
      /* modulators are never replaced */
      return 0;
    }

  for (int j = 0; j < self->deleted_ms->num_slots;
       j++)
    {
      int slot_to_revert =
        self->deleted_ms->slots[j];
      if (slot_to_revert != to_slot)
        {
          continue;
        }

      Plugin * deleted_pl =
        self->deleted_ms->plugins[j];
      g_message (
        "reverting plugin %s in slot %d",
        deleted_pl->setting->descr->name,
        slot_to_revert);

      /* note: this also instantiates the
       * plugin */
      GError * err = NULL;
      Plugin * new_pl =
        plugin_clone (deleted_pl, &err);
      if (!new_pl)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "%s",
            _ ("Failed to clone plugin"));
          return -1;
        }

      /* add to channel */
      track_insert_plugin (
        to_tr, new_pl, self->deleted_ms->type,
        slot_to_revert, Z_F_INSTANTIATE,
        F_REPLACING, F_NOT_MOVING_PLUGIN,
        F_NO_CONFIRM, F_GEN_AUTOMATABLES,
        F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);

      /* bring back automation */
      revert_automation (
        self, to_tr, self->deleted_ms,
        slot_to_revert, true);

      /* activate and set visibility */
      plugin_activate (new_pl, F_ACTIVATE);

      /* show if was visible before */
      if (
        ZRYTHM_HAVE_UI
        && self->deleted_ms->plugins[j]->visible)
        {
          new_pl->visible = true;
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, new_pl);
        }
    }

  return 0;
}

static int
do_or_undo_create_or_delete (
  MixerSelectionsAction * self,
  bool                    _do,
  bool                    create,
  GError **               error)
{
  Track * track = NULL;
  if (create)
    track = tracklist_find_track_by_name_hash (
      TRACKLIST, self->to_track_name_hash);
  else
    track = tracklist_find_track_by_name_hash (
      TRACKLIST, self->ms_before->track_name_hash);
  g_return_val_if_fail (track, -1);

  Channel *         ch = track->channel;
  MixerSelections * own_ms = self->ms_before;
  PluginSlotType    slot_type =
    create ? self->slot_type : own_ms->type;
  int loop_times =
    create && self->type != MIXER_SELECTIONS_ACTION_PASTE
      ? self->num_plugins
      : own_ms->num_slots;
  bool delete = !create;

  /* if creating plugins (create do or delete undo) */
  if ((create && _do) || (delete &&!_do))
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
            create
              ? (self->to_slot + i)
              : own_ms->plugins[i]->id.slot;

          /* create new plugin */
          Plugin * pl = NULL;
          if (create)
            {
              GError * err = NULL;
              if (
                self->type
                == MIXER_SELECTIONS_ACTION_PASTE)
                {
                  pl = plugin_clone (
                    own_ms->plugins[i], &err);
                }
              else
                {
                  pl = plugin_new_from_setting (
                    self->setting,
                    self->to_track_name_hash,
                    slot_type, slot, &err);
                }
              if (!IS_PLUGIN_AND_NONNULL (pl))
                {
                  HANDLE_ERROR (
                    err, "%s",
                    _ ("Could not create plugin"));
                  return -1;
                }

              /* instantiate so that ports are
               * created */
              int ret = plugin_instantiate (
                pl, NULL, &err);
              if (ret != 0)
                {
                  HANDLE_ERROR (
                    err, "%s",
                    _ ("Failed to instantiate "
                       "plugin"));
                  return -1;
                }
            }
          else if (delete)
            {
              /* note: this also instantiates the
               * plugin */
              GError * err = NULL;
              pl = plugin_clone (
                own_ms->plugins[i], &err);
              if (!IS_PLUGIN_AND_NONNULL (pl))
                {
                  if (err)
                    {
                      g_warning (
                        "Failed to clone plugin: "
                        "%s",
                        err->message);
                      g_error_free_and_null (err);
                    }
                  return -1;
                }
            }

          /* validate */
          g_return_val_if_fail (pl, -1);
          if (delete)
            {
              g_return_val_if_fail (
                slot == own_ms->slots[i], -1);
            }

          /* set track */
          pl->track = track;
          plugin_set_track_name_hash (
            pl, track_get_name_hash (track));

          /* save any plugin about to be deleted */
          save_existing_plugin (
            self, self->deleted_ms, NULL,
            slot_type, -1,
            slot_type == PLUGIN_SLOT_MODULATOR
              ? P_MODULATOR_TRACK
              : track,
            slot_type, slot);

          /* add to destination */
          track_insert_plugin (
            track, pl, slot_type, slot,
            Z_F_INSTANTIATE, F_NOT_REPLACING,
            F_NOT_MOVING_PLUGIN, F_NO_CONFIRM,
            F_GEN_AUTOMATABLES, F_NO_RECALC_GRAPH,
            F_NO_PUBLISH_EVENTS);

          /* select the plugin */
          mixer_selections_add_slot (
            MIXER_SELECTIONS, track, slot_type,
            pl->id.slot, F_NO_CLONE);

          /* set visibility */
          if (create)
            {
              /* set visible from settings */
              pl->visible =
                ZRYTHM_HAVE_UI
                && g_settings_get_boolean (
                  S_P_PLUGINS_UIS,
                  "open-on-instantiate");
            }
          else if (delete)
            {
              /* set visible if plugin was visible
               * before deletion */
              pl->visible =
                ZRYTHM_HAVE_UI
                && own_ms->plugins[i]->visible;
            }
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, pl);

          /* activate */
          int ret =
            plugin_activate (pl, F_ACTIVATE);
          g_return_val_if_fail (!ret, -1);
        }

      /* if undoing deletion */
      if (delete)
        {
          for (int i = 0; i < loop_times; i++)
            {
              /* restore port connections */
              Plugin * pl = own_ms->plugins[i];
              g_message (
                "restoring custom connections "
                "for plugin '%s'",
                pl->setting->descr->name);
              GPtrArray * ports =
                g_ptr_array_new ();
              plugin_append_ports (pl, ports);
              for (size_t j = 0; j < ports->len; j++)
                {
                  Port * port =
                    g_ptr_array_index (ports, j);
                  Port * prj_port =
                    port_find_from_identifier (
                      &port->id);
                  port_restore_from_non_project (
                    prj_port, port);
                }
              object_free_w_func_and_null (
                g_ptr_array_unref, ports);

              /* copy automation from before
               * deletion */
              int slot = pl->id.slot;
              revert_automation (
                self, track, own_ms, slot, false);
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
            create
              ? (self->to_slot + i)
              : own_ms->plugins[i]->id.slot;

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
              GPtrArray * ports =
                g_ptr_array_new ();
              plugin_append_ports (prj_pl, ports);
              GPtrArray * own_ports =
                g_ptr_array_new ();
              plugin_append_ports (
                own_pl, own_ports);
              for (size_t j = 0; j < ports->len; j++)
                {
                  Port * prj_port =
                    g_ptr_array_index (ports, j);

                  Port * own_port = NULL;
                  for (size_t k = 0;
                       k < own_ports->len; k++)
                    {
                      Port * cur_own_port =
                        g_ptr_array_index (
                          own_ports, k);
                      if (port_identifier_is_equal (
                            &cur_own_port->id,
                            &prj_port->id))
                        {
                          own_port = cur_own_port;
                          break;
                        }
                    }
                  g_return_val_if_fail (
                    own_port, -1);

                  port_copy_metadata_from_project (
                    own_port, prj_port);
                }
              object_free_w_func_and_null (
                g_ptr_array_unref, ports);
              object_free_w_func_and_null (
                g_ptr_array_unref, own_ports);
            }

          /* remove the plugin at given slot */
          track_remove_plugin (
            track, slot_type, slot,
            F_NOT_REPLACING, F_NOT_MOVING_PLUGIN,
            F_DELETING_PLUGIN, F_NOT_DELETING_TRACK,
            F_NO_RECALC_GRAPH);

          /* if there was a plugin at the slot
           * before, bring it back */
          GError * err = NULL;
          int      ret = revert_deleted_plugin (
                 self, track, slot, &err);
          if (ret != 0)
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, "%s",
                _ ("Failed to revert deleted "
                   "plugin"));
              return -1;
            }
        }

      EVENTS_PUSH (ET_PLUGINS_REMOVED, NULL);
    }

  /* restore connections */
  reset_port_connections (self, _do);

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  if (ch)
    {
      EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, ch);
    }

  return 0;
}

static void
copy_automation_from_track1_to_track2 (
  Track *        from_track,
  Track *        to_track,
  PluginSlotType slot_type,
  int            from_slot,
  int            to_slot)
{
  AutomationTracklist * prev_atl =
    track_get_automation_tracklist (from_track);
  for (int j = 0; j < prev_atl->num_ats; j++)
    {
      /* get the previous at */
      AutomationTrack * prev_at = prev_atl->ats[j];
      if (
        prev_at->num_regions == 0
        || prev_at->port_id.owner_type
             != PORT_OWNER_TYPE_PLUGIN
        || prev_at->port_id.plugin_id.slot != from_slot
        || prev_at->port_id.plugin_id.slot_type
             != slot_type)
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

          if (
            at->port_id.owner_type
              != PORT_OWNER_TYPE_PLUGIN
            || at->port_id.plugin_id.slot != to_slot
            || at->port_id.plugin_id.slot_type
                 != slot_type
            || at->port_id.port_index
                 != prev_at->port_id.port_index)
            {
              continue;
            }

          /* copy the automation regions */
          for (int l = 0; l < prev_at->num_regions;
               l++)
            {
              ZRegion * prev_region =
                prev_at->regions[l];
              ZRegion * new_region =
                (ZRegion *) arranger_object_clone (
                  (ArrangerObject *) prev_region);
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
  bool                    copy,
  GError **               error)
{
  MixerSelections * own_ms = self->ms_before;
  PluginSlotType    from_slot_type = own_ms->type;
  PluginSlotType    to_slot_type = self->slot_type;
  Track *           from_tr =
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
          char * str = g_strdup_printf (
            "%s (Copy)",
            own_pl->setting->descr->name);
          to_tr = track_new (
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
          self->to_track_name_hash =
            track_get_name_hash (to_tr);
        }
      /* else if not new track/channel */
      else
        {
          to_tr = tracklist_find_track_by_name_hash (
            TRACKLIST, self->to_track_name_hash);
        }

      Channel * to_ch = to_tr->channel;
      g_return_val_if_fail (IS_CHANNEL (to_ch), -1);

      mixer_selections_clear (
        MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

      /* sort own selections */
      mixer_selections_sort (own_ms, F_ASCENDING);

      bool move_downwards_same_track =
        to_tr == from_tr && own_ms->num_slots > 0
        && self->to_slot
             > own_ms->plugins[0]->id.slot;
#define FOREACH_SLOT \
  for ( \
    int i = \
      move_downwards_same_track \
        ? own_ms->num_slots - 1 \
        : 0; \
    move_downwards_same_track \
      ? (i >= 0) \
      : (i < own_ms->num_slots); \
    move_downwards_same_track ? i-- : i++)

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

      FOREACH_SLOT
      {
        /* get/create the actual plugin */
        int from_slot = own_ms->plugins[i]->id.slot;
        Plugin * pl = NULL;
        if (move)
          {
            pl = track_get_plugin_at_slot (
              from_tr, own_ms->type, from_slot);
            g_return_val_if_fail (
              IS_PLUGIN_AND_NONNULL (pl)
                && pl->id.track_name_hash
                     == track_get_name_hash (from_tr),
              -1);
          }
        else
          {
            GError * err = NULL;
            pl = plugin_clone (
              own_ms->plugins[i], &err);
            if (!IS_PLUGIN_AND_NONNULL (pl))
              {
                if (err)
                  {
                    g_warning (
                      "Could not create plugin: "
                      "%s",
                      err->message);
                    g_error_free (err);
                  }
                return -1;
              }
          }

        int to_slot = self->to_slot + i;

        /* save any plugin about to be deleted */
        save_existing_plugin (
          self, self->deleted_ms, from_tr,
          from_slot_type, from_slot, to_tr,
          to_slot_type, to_slot);

        /* move or copy the plugin */
        if (move)
          {
            g_debug (
              "%s: moving plugin from "
              "%s:%s:%d to %s:%s:%d",
              __func__, from_tr->name,
              plugin_slot_type_strings[from_slot_type]
                .str,
              from_slot, to_tr->name,
              plugin_slot_type_strings[to_slot_type]
                .str,
              to_slot);

            if (
              from_tr != to_tr
              || from_slot_type != to_slot_type
              || from_slot != to_slot)
              {
                plugin_move (
                  pl, to_tr, to_slot_type, to_slot,
                  F_NO_PUBLISH_EVENTS);
              }
          }
        else if (copy)
          {
            g_debug (
              "%s: copying plugin from "
              "%s:%s:%d to %s:%s:%d",
              __func__, from_tr->name,
              plugin_slot_type_strings[from_slot_type]
                .str,
              from_slot, to_tr->name,
              plugin_slot_type_strings[to_slot_type]
                .str,
              to_slot);

            track_insert_plugin (
              to_tr, pl, to_slot_type, to_slot,
              Z_F_INSTANTIATE, F_NOT_REPLACING,
              F_NOT_MOVING_PLUGIN, F_NO_CONFIRM,
              F_GEN_AUTOMATABLES, F_NO_RECALC_GRAPH,
              F_NO_PUBLISH_EVENTS);

            g_return_val_if_fail (
              pl->num_in_ports
                == own_ms->plugins[i]->num_in_ports,
              -1);
          }

        /* copy automation regions from original
           * plugin */
        if (copy)
          {
            copy_automation_from_track1_to_track2 (
              from_tr, to_tr, to_slot_type,
              own_ms->slots[i], to_slot);
          }

        /* select it */
        mixer_selections_add_slot (
          MIXER_SELECTIONS, to_tr, to_slot_type,
          to_slot, F_NO_CLONE);

        /* if new plugin (copy), instantiate it,
           * activate it and set visibility */
        if (copy)
          {
            g_return_val_if_fail (
              plugin_activate (pl, F_ACTIVATE) == 0,
              -1);

            /* show if was visible before */
            if (
              ZRYTHM_HAVE_UI
              && own_ms->plugins[i]->visible)
              {
                pl->visible = true;
                EVENTS_PUSH (
                  ET_PLUGIN_VISIBILITY_CHANGED, pl);
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
        tracklist_find_track_by_name_hash (
          TRACKLIST, self->to_track_name_hash);
      Channel * to_ch = to_tr->channel;
      g_return_val_if_fail (IS_TRACK (to_tr), -1);

      /* clear selections to readd each original
       * plugin */
      mixer_selections_clear (
        MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

      /* sort own selections */
      mixer_selections_sort (own_ms, F_ASCENDING);

      bool move_downwards_same_track =
        to_tr == from_tr && own_ms->num_slots > 0
        && self->to_slot
             < own_ms->plugins[0]->id.slot;
      for (
        int i =
          move_downwards_same_track
            ? own_ms->num_slots - 1
            : 0;
        move_downwards_same_track
          ? (i >= 0)
          : (i < own_ms->num_slots);
        move_downwards_same_track ? i-- : i++)
        {
          /* get the actual plugin */
          int      to_slot = self->to_slot + i;
          Plugin * pl = track_get_plugin_at_slot (
            to_tr, to_slot_type, to_slot);
          g_return_val_if_fail (IS_PLUGIN (pl), -1);

          /* original slot */
          int from_slot =
            own_ms->plugins[i]->id.slot;

          /* if moving plugins back */
          if (move)
            {
              /* move plugin to its original
               * slot */
              g_debug (
                "%s: moving plugin back from "
                "%s:%s:%d to %s:%s:%d",
                __func__, to_tr->name,
                plugin_slot_type_strings[to_slot_type]
                  .str,
                to_slot, from_tr->name,
                plugin_slot_type_strings[from_slot_type]
                  .str,
                from_slot);

              if (
                from_tr != to_tr
                || from_slot_type != to_slot_type
                || from_slot != to_slot)
                {
                  Plugin * existing_pl =
                    track_get_plugin_at_slot (
                      from_tr, from_slot_type,
                      from_slot);
                  g_warn_if_fail (!existing_pl);
                  plugin_move (
                    pl, from_tr, from_slot_type,
                    from_slot, F_NO_PUBLISH_EVENTS);
                }
            }
          else if (copy)
            {
              track_remove_plugin (
                to_tr, to_slot_type, to_slot,
                F_NOT_REPLACING,
                F_NOT_MOVING_PLUGIN,
                F_DELETING_PLUGIN,
                F_NOT_DELETING_TRACK,
                F_NO_RECALC_GRAPH);
              pl = NULL;
            }

          /* if there was a plugin at the slot
           * before, bring it back */
          GError * err = NULL;
          int      ret = revert_deleted_plugin (
                 self, to_tr, to_slot, &err);
          if (ret != 0)
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, "%s",
                _ ("Failed to revert deleted "
                   "plugin"));
              return -1;
            }

          if (copy)
            {
              pl = track_get_plugin_at_slot (
                from_tr, from_slot_type, from_slot);
            }

          /* add orig plugin to mixer selections */
          g_warn_if_fail (
            IS_PLUGIN_AND_NONNULL (pl));
          mixer_selections_add_slot (
            MIXER_SELECTIONS, from_tr,
            from_slot_type, from_slot, F_NO_CLONE);
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

  /* restore connections */
  reset_port_connections (self, _do);

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  return 0;
}

static int
do_or_undo (
  MixerSelectionsAction * self,
  bool                    _do,
  GError **               error)
{
  switch (self->type)
    {
    case MIXER_SELECTIONS_ACTION_CREATE:
      return do_or_undo_create_or_delete (
        self, _do, true, error);
    case MIXER_SELECTIONS_ACTION_DELETE:
      return do_or_undo_create_or_delete (
        self, _do, false, error);
    case MIXER_SELECTIONS_ACTION_MOVE:
      return do_or_undo_move_or_copy (
        self, _do, false, error);
      break;
    case MIXER_SELECTIONS_ACTION_COPY:
      return do_or_undo_move_or_copy (
        self, _do, true, error);
      break;
    case MIXER_SELECTIONS_ACTION_PASTE:
      return do_or_undo_create_or_delete (
        self, _do, true, error);
      break;
    default:
      g_warn_if_reached ();
    }

  /* if first do and keeping track of connections,
   * clone the new connections */
  if (
    _do && self->connections_mgr_before
    && !self->connections_mgr_after)
    self->connections_mgr_after =
      port_connections_manager_clone (
        PORT_CONNECTIONS_MGR);

  return -1;
}

int
mixer_selections_action_do (
  MixerSelectionsAction * self,
  GError **               error)
{
  return do_or_undo (self, true, error);
}

int
mixer_selections_action_undo (
  MixerSelectionsAction * self,
  GError **               error)
{
  return do_or_undo (self, false, error);
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
            _ ("Create %s"),
            self->setting->descr->name);
        }
      else
        {
          return g_strdup_printf (
            _ ("Create %d %ss"), self->num_plugins,
            self->setting->descr->name);
        }
    case MIXER_SELECTIONS_ACTION_DELETE:
      if (self->ms_before->num_slots == 1)
        {
          return g_strdup_printf (
            _ ("Delete Plugin"));
        }
      else
        {
          return g_strdup_printf (
            _ ("Delete %d Plugins"),
            self->ms_before->num_slots);
        }
    case MIXER_SELECTIONS_ACTION_MOVE:
      if (self->ms_before->num_slots == 1)
        {
          return g_strdup_printf (
            _ ("Move %s"),
            self->ms_before->plugins[0]
              ->setting->descr->name);
        }
      else
        {
          return g_strdup_printf (
            _ ("Move %d Plugins"),
            self->ms_before->num_slots);
        }
    case MIXER_SELECTIONS_ACTION_COPY:
      if (self->ms_before->num_slots == 1)
        {
          return g_strdup_printf (
            _ ("Copy %s"),
            self->ms_before->plugins[0]
              ->setting->descr->name);
        }
      else
        {
          return g_strdup_printf (
            _ ("Copy %d Plugins"),
            self->ms_before->num_slots);
        }
    case MIXER_SELECTIONS_ACTION_PASTE:
      if (self->ms_before->num_slots == 1)
        {
          return g_strdup_printf (
            _ ("Paste %s"),
            self->ms_before->plugins[0]
              ->setting->descr->name);
        }
      else
        {
          return g_strdup_printf (
            _ ("Paste %d Plugins"),
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
