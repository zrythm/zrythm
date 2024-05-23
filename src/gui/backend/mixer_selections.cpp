// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/mixer_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/yaml.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

void
mixer_selections_init_loaded (MixerSelections * self, bool is_project)
{
  if (!is_project)
    {
      for (int i = 0; i < self->num_slots; i++)
        {
          plugin_init_loaded (self->plugins[i], NULL, self);
        }
    }

  /* sort the selections */
  mixer_selections_sort (self, F_ASCENDING);
}

void
mixer_selections_init (MixerSelections * self)
{
  self->schema_version = MIXER_SELECTIONS_SCHEMA_VERSION;
}

MixerSelections *
mixer_selections_new (void)
{
  MixerSelections * self = object_new (MixerSelections);

  mixer_selections_init (self);

  return self;
}

/**
 * Returns if there are any selections.
 */
int
mixer_selections_has_any (MixerSelections * self)
{
  return self->has_any;
}

static int
sort_plugins_asc_func (const void * _a, const void * _b)
{
  Plugin * a = *(Plugin * const *) _a;
  Plugin * b = *(Plugin * const *) _b;
  return a->id.slot - b->id.slot;
}

static int
sort_plugins_desc_func (const void * _a, const void * _b)
{
  return -sort_plugins_asc_func (_a, _b);
}

static int
sort_slots_asc_func (const void * _a, const void * _b)
{
  int a = *(int const *) _a;
  int b = *(int const *) _b;
  return a - b;
}

static int
sort_slots_desc_func (const void * _a, const void * _b)
{
  return -sort_slots_asc_func (_a, _b);
}

/**
 * Sorts the selections by slot index.
 *
 * @param asc Ascending or not.
 */
void
mixer_selections_sort (MixerSelections * self, bool asc)
{
  if (!self->has_any)
    {
      return;
    }

  qsort (
    self->slots, (size_t) self->num_slots, sizeof (int),
    asc ? sort_slots_asc_func : sort_slots_desc_func);
  qsort (
    self->plugins, (size_t) self->num_slots, sizeof (Plugin *),
    asc ? sort_plugins_asc_func : sort_plugins_desc_func);
}

/**
 * Gets highest slot in the selections.
 */
int
mixer_selections_get_highest_slot (MixerSelections * ms)
{
  int min = STRIP_SIZE;

  for (int i = 0; i < ms->num_slots; i++)
    {
      if (ms->slots[i] < min)
        min = ms->slots[i];
    }

  g_warn_if_fail (min < STRIP_SIZE);

  return min;
}

/**
 * Gets lowest slot in the selections.
 */
int
mixer_selections_get_lowest_slot (MixerSelections * ms)
{
  int max = -1;

  for (int i = 0; i < ms->num_slots; i++)
    {
      if (ms->slots[i] > max)
        max = ms->slots[i];
    }

  g_warn_if_fail (max > -1);

  return max;
}

/**
 * Get current Track.
 */
Track *
mixer_selections_get_track (const MixerSelections * const self)
{
  if (!self->has_any)
    return NULL;

  Track * track =
    tracklist_find_track_by_name_hash (TRACKLIST, self->track_name_hash);
  g_return_val_if_fail (track, NULL);
  return track;
}

/**
 * Adds a slot to the selections.
 *
 * The selections can only be from one channel.
 *
 * @param track The track.
 * @param slot The slot to add to the selections.
 * @param clone_pl Whether to clone the plugin
 *   when storing it in \ref
 *   MixerSelections.plugins. Used in some actions.
 */
void
mixer_selections_add_slot (
  MixerSelections * ms,
  Track *           track,
  ZPluginSlotType   type,
  int               slot,
  bool              clone_pl,
  const bool        fire_events)
{
  unsigned int name_hash = track_get_name_hash (track);

  if (!ms->has_any || name_hash != ms->track_name_hash || type != ms->type)
    {
      mixer_selections_clear (ms, F_NO_PUBLISH_EVENTS);
    }
  ms->track_name_hash = name_hash;
  ms->type = type;
  ms->has_any = true;

  g_debug ("adding slot %s:%s:%d", track->name, ENUM_NAME (type), slot);

  Plugin * pl = NULL;
  if (!array_contains_int (ms->slots, ms->num_slots, slot))
    {
      switch (type)
        {
        case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
          pl = track->channel->midi_fx[slot];
          break;
        case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
          pl = track->channel->inserts[slot];
          break;
        case ZPluginSlotType::Z_PLUGIN_SLOT_MODULATOR:
          pl = track->modulators[slot];
          break;
        case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
          pl = track->channel->instrument;
          break;
        default:
          break;
        }
      g_return_if_fail (pl);

      Plugin * pl_to_append = pl;
      if (clone_pl)
        {
          GError * err = NULL;
          pl_to_append = plugin_clone (pl, &err);
          if (!pl_to_append)
            {
              /* FIXME handle properly */
              g_critical ("failed to clone plugin: %s", err->message);
              return;
            }
        }

      array_double_append (
        ms->slots, ms->plugins, ms->num_slots, slot, pl_to_append);
    }

  if (pl && plugin_is_in_active_project (pl))
    {
      EVENTS_PUSH (EventType::ET_MIXER_SELECTIONS_CHANGED, pl);
    }
}

/**
 * Removes a slot from the selections.
 *
 * Assumes that the channel is the one already
 * selected.
 */
void
mixer_selections_remove_slot (
  MixerSelections * ms,
  int               slot,
  ZPluginSlotType   type,
  bool              publish_events)
{
  g_message ("removing slot %d", slot);
  array_delete_primitive (ms->slots, ms->num_slots, slot);

  if (
    ms->num_slots == 0 || ms->type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    {
      ms->has_any = 0;
      ms->track_name_hash = 0;
    }

  if (ZRYTHM_HAVE_UI && publish_events)
    {
      EVENTS_PUSH (EventType::ET_MIXER_SELECTIONS_CHANGED, NULL);
    }
}

/**
 * Returns if the slot is selected or not.
 */
bool
mixer_selections_contains_slot (
  MixerSelections * self,
  ZPluginSlotType   type,
  int               slot)
{
  if (type != self->type)
    return false;

  if (type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    {
      return self->has_any;
    }
  else
    {
      for (int i = 0; i < self->num_slots; i++)
        {
          if (self->slots[i] == slot)
            {
              return true;
            }
        }
    }

  return false;
}

/**
 * Returns if the plugin is selected or not.
 */
bool
mixer_selections_contains_plugin (MixerSelections * ms, Plugin * pl)
{
  g_return_val_if_fail (ms && IS_PLUGIN (pl), false);

  if (ms->track_name_hash != pl->id.track_name_hash)
    return false;

  if (ms->type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    {
      if (
        pl->id.slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT
        && pl->id.track_name_hash == ms->track_name_hash)
        {
          return true;
        }
    }
  else
    {
      for (int i = 0; i < ms->num_slots; i++)
        {
          if (ms->slots[i] == pl->id.slot && ms->type == pl->id.slot_type)
            return true;
        }
    }

  return false;
}

bool
mixer_selections_contains_uninstantiated_plugin (
  const MixerSelections * const self)
{
  GPtrArray * arr = g_ptr_array_new ();

  mixer_selections_get_plugins (self, arr, false);

  bool ret = false;
  for (size_t i = 0; i < arr->len; i++)
    {
      Plugin * pl = (Plugin *) g_ptr_array_index (arr, i);

      if (pl->instantiation_failed)
        {
          ret = true;
          break;
        }
    }

  g_ptr_array_unref (arr);

  return ret;
}

/**
 * Returns the first selected plugin if any is
 * selected, otherwise NULL.
 */
Plugin *
mixer_selections_get_first_plugin (MixerSelections * self)
{
  if (self->has_any)
    {
      Track * track = mixer_selections_get_track (self);
      g_return_val_if_fail (track, NULL);
      switch (self->type)
        {
        case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
          return track->channel->instrument;
        case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
          return track->channel->inserts[self->slots[0]];
        case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
          return track->channel->midi_fx[self->slots[0]];
        case ZPluginSlotType::Z_PLUGIN_SLOT_MODULATOR:
          return track->modulators[self->slots[0]];
        default:
          g_return_val_if_reached (NULL);
          break;
        }
    }

  return NULL;
}

/**
 * Fills in the array with the plugins in the
 * selections.
 */
int
mixer_selections_get_plugins (
  const MixerSelections * const self,
  GPtrArray *                   arr,
  bool                          from_cache)
{
  if (!self->has_any)
    return 0;

  if (from_cache)
    {
      for (int i = 0; i < self->num_slots; i++)
        {
          Plugin * pl = self->plugins[i];
          if (pl)
            {
              g_ptr_array_add (arr, pl);
            }
        }
    }
  else
    {
      Track * track = mixer_selections_get_track (self);
      g_return_val_if_fail (IS_TRACK_AND_NONNULL (track), false);

      for (int i = 0; i < self->num_slots; i++)
        {
          Plugin * pl = NULL;
          switch (self->type)
            {
            case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
              pl = track->channel->instrument;
              break;
            case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
              pl = track->channel->inserts[self->slots[i]];
              break;
            case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
              pl = track->channel->midi_fx[self->slots[i]];
              break;
            case ZPluginSlotType::Z_PLUGIN_SLOT_MODULATOR:
              pl = track->modulators[self->slots[i]];
              break;
            default:
              g_return_val_if_reached (false);
              break;
            }

          g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), 0);

          g_ptr_array_add (arr, pl);
        }
    }

  return self->num_slots;
}

bool
mixer_selections_validate (MixerSelections * self)
{
  if (!self->has_any)
    return true;

  Track * track = mixer_selections_get_track (self);
  g_return_val_if_fail (IS_TRACK_AND_NONNULL (track), false);

  for (int i = 0; i < self->num_slots; i++)
    {
      Plugin * pl = NULL;
      switch (self->type)
        {
        case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
          pl = track->channel->instrument;
          break;
        case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
          pl = track->channel->inserts[self->slots[i]];
          break;
        case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
          pl = track->channel->midi_fx[self->slots[i]];
          break;
        case ZPluginSlotType::Z_PLUGIN_SLOT_MODULATOR:
          pl = track->modulators[self->slots[i]];
          break;
        default:
          g_return_val_if_reached (false);
          break;
        }

      g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), false);
    }

  return true;
}

/**
 * Clears selections.
 */
void
mixer_selections_clear (MixerSelections * self, const int pub_events)
{
  int i;

  for (i = self->num_slots - 1; i >= 0; i--)
    {
      mixer_selections_remove_slot (
        self, self->slots[i], self->type, F_NO_PUBLISH_EVENTS);
    }

  self->has_any = false;
  self->track_name_hash = 0;

  if (pub_events)
    {
      EVENTS_PUSH (EventType::ET_MIXER_SELECTIONS_CHANGED, NULL);
    }

  g_message ("cleared mixer selections");
}

/**
 * Clone the struct for copying, undoing, etc.
 *
 * @bool src_is_project Whether \ref src are the
 *   project selections.
 */
MixerSelections *
mixer_selections_clone (const MixerSelections * src, bool src_is_project)
{
  MixerSelections * ms = mixer_selections_new ();

  int i;

  for (i = 0; i < src->num_slots; i++)
    {
      Plugin * pl = NULL;
      if (src_is_project)
        {
          Track * track =
            tracklist_find_track_by_name_hash (TRACKLIST, src->track_name_hash);
          pl = track_get_plugin_at_slot (track, src->type, src->slots[i]);
          g_return_val_if_fail (IS_PLUGIN (pl), NULL);
        }
      else
        {
          pl = src->plugins[i];
        }

      GError * err = NULL;
      ms->plugins[i] = plugin_clone (pl, &err);
      if (!ms->plugins[i])
        {
          g_warning (
            "failed to clone plugin %s: %s", pl->setting->descr->name,
            err->message);
          g_error_free_and_null (err);
          mixer_selections_free (ms);
          return NULL;
        }
      ms->slots[i] = src->slots[i];
    }

  ms->type = src->type;
  ms->num_slots = src->num_slots;
  ms->has_any = src->has_any;
  ms->track_name_hash = src->track_name_hash;

  return ms;
}

void
mixer_selections_post_deserialize (MixerSelections * self)
{
  for (int i = 0; i < self->num_slots; i++)
    {
      plugin_init_loaded (self->plugins[i], NULL, self);
    }

  /* sort the selections */
  mixer_selections_sort (self, F_ASCENDING);
}

/**
 * Returns whether the selections can be pasted to
 * MixerWidget.paste_slot.
 */
bool
mixer_selections_can_be_pasted (
  MixerSelections * self,
  Channel *         ch,
  ZPluginSlotType   type,
  int               slot)
{
  int lowest = mixer_selections_get_lowest_slot (self);
  int highest = mixer_selections_get_highest_slot (self);
  int delta = lowest - highest;

  return slot + delta < STRIP_SIZE;
}

/**
 * Paste the selections starting at the slot in the
 * given channel.
 */
void
mixer_selections_paste_to_slot (
  MixerSelections * ms,
  Channel *         ch,
  ZPluginSlotType   type,
  int               slot)
{
  GError * err = NULL;
  bool     ret = mixer_selections_action_perform_paste (
    ms, PORT_CONNECTIONS_MGR, type, track_get_name_hash (ch->track), slot, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to paste plugins"));
    }
}

void
mixer_selections_free (MixerSelections * self)
{
  free (self);
}
