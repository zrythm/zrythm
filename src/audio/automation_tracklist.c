// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdlib.h>

#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

/**
 * Inits a loaded AutomationTracklist.
 */
void
automation_tracklist_init_loaded (
  AutomationTracklist * self,
  Track *               track)
{
  self->track = track;
  self->ats_size = (size_t) self->num_ats;
  int               j;
  AutomationTrack * at;
  for (j = 0; j < self->num_ats; j++)
    {
      at = self->ats[j];
      automation_track_init_loaded (at, self);
    }
}

Track *
automation_tracklist_get_track (AutomationTracklist * self)
{
  g_return_val_if_fail (self->track, NULL);

  return self->track;
}

void
automation_tracklist_add_at (
  AutomationTracklist * self,
  AutomationTrack *     at)
{
#if 0
  g_debug (
    "[track %d atl] adding automation track at: "
    "%d '%s'",
    self->track_pos, self->num_ats,
    at->port_id.label);
#endif

  array_double_size_if_full (
    self->ats, self->num_ats, self->ats_size,
    AutomationTrack *);
  array_append (self->ats, self->num_ats, at);

  at->index = self->num_ats - 1;
  at->port_id.track_name_hash =
    track_get_name_hash (self->track);

  /* move automation track regions */
  for (int i = 0; i < at->num_regions; i++)
    {
      ZRegion * region = at->regions[i];
      region_set_automation_track (region, at);
    }
}

/**
 * Gets the automation track matching the given
 * arguments.
 *
 * Currently only used in mixer selections action.
 */
AutomationTrack *
automation_tracklist_get_plugin_at (
  AutomationTracklist * self,
  PluginSlotType        slot_type,
  const int             plugin_slot,
  const int             port_index,
  const char *          symbol)
{
  for (int i = 0; i < self->num_ats; i++)
    {
      AutomationTrack * at = self->ats[i];
      g_return_val_if_fail (at, NULL);

      if (
        at->port_id.owner_type == PORT_OWNER_TYPE_PLUGIN
        && plugin_slot == at->port_id.plugin_id.slot
        && slot_type == at->port_id.plugin_id.slot_type
        && port_index == at->port_id.port_index
        && string_is_equal (symbol, at->port_id.sym))
        {
          return at;
        }
    }
  g_return_val_if_reached (NULL);
}

/**
 * Adds all automation tracks and sets fader as
 * visible.
 */
void
automation_tracklist_init (
  AutomationTracklist * self,
  Track *               track)
{
  self->track = track;

  g_message ("initing automation tracklist...");
  self->schema_version = AUTOMATION_TRACKLIST_SCHEMA_VERSION;

  /*self->ats_size = 1;*/
  /*self->ats =*/
  /*object_new_n (self->ats_size, AutomationTrack *);*/
}

/**
 * Sets the index of the AutomationTrack and swaps
 * it with the AutomationTrack at that index or
 * pushes the other AutomationTrack's down.
 *
 * A special case is when \ref index == \ref
 * AutomationTracklist.num_ats. In this case, the
 * given automation track is set last and all the
 * other automation tracks are pushed upwards.
 *
 * @param push_down False to swap positions with the
 *   current AutomationTrack, or true to push down
 *   all the tracks below.
 */
void
automation_tracklist_set_at_index (
  AutomationTracklist * self,
  AutomationTrack *     at,
  int                   index,
  bool                  push_down)
{
  /* special case */
  if (index == self->num_ats && push_down)
    {
      /* move AT to before last */
      automation_tracklist_set_at_index (
        self, at, index - 1, push_down);
      /* move last AT to before last */
      automation_tracklist_set_at_index (
        self, self->ats[self->num_ats - 1], index - 1,
        push_down);
      return;
    }

  g_return_if_fail (index < self->num_ats && self->ats[index]);

  /*g_message ("setting %s (%d) to %d",*/
  /*at->automatable->label, at->index,*/
  /*index);*/

  if (at->index == index)
    return;

  ZRegion * clip_editor_region =
    clip_editor_get_region (CLIP_EDITOR);
  int clip_editor_region_idx = -2;
  if (clip_editor_region)
    {
      RegionIdentifier * clip_editor_region_id =
        &clip_editor_region->id;

      if (
        clip_editor_region_id->track_name_hash
        == at->port_id.track_name_hash)
        {
          clip_editor_region_idx =
            clip_editor_region_id->at_idx;
        }
    }

  if (push_down)
    {
      /* whether the new index is before the current
       * index (the index of automation tracks in
       * between needs to increase) */
      bool increase = at->index > index;
      if (increase)
        {
          for (int i = at->index - 1; i >= index; i--)
            {
              int prev_idx = i;
              int new_idx = i + 1;
              self->ats[new_idx] = self->ats[i];
              automation_track_set_index (
                self->ats[i], new_idx);

              /* update clip editor region if it was
               * affected */
              if (clip_editor_region_idx == prev_idx)
                {
                  CLIP_EDITOR->region_id.at_idx = new_idx;
                }
            }

          int prev_idx = at->index;
          self->ats[index] = at;
          automation_track_set_index (at, index);

          /* update clip editor region if it was
           * affected */
          if (clip_editor_region_idx == prev_idx)
            {
              CLIP_EDITOR->region_id.at_idx = index;
            }
        }
      else
        {
          for (int i = at->index + 1; i <= index; i++)
            {
              int prev_idx = i;
              int new_idx = i - 1;
              self->ats[new_idx] = self->ats[i];
              automation_track_set_index (
                self->ats[i], new_idx);

              /* update clip editor region if it was
               * affected */
              if (clip_editor_region_idx == prev_idx)
                {
                  CLIP_EDITOR->region_id.at_idx = new_idx;
                }
            }

          int prev_idx = at->index;
          self->ats[index] = at;
          automation_track_set_index (at, index);

          /* update clip editor region if it was
           * affected */
          if (clip_editor_region_idx == prev_idx)
            {
              CLIP_EDITOR->region_id.at_idx = index;
            }
        }
    }
  else
    {
      int               prev_index = at->index;
      AutomationTrack * new_at = self->ats[index];
      self->ats[index] = at;
      automation_track_set_index (at, index);

      self->ats[prev_index] = new_at;
      automation_track_set_index (new_at, prev_index);

      /* update clip editor region if it was
       * affected */
      if (clip_editor_region_idx == index)
        {
          CLIP_EDITOR->region_id.at_idx = prev_index;
        }
      else if (clip_editor_region_idx == prev_index)
        {
          CLIP_EDITOR->region_id.at_idx = index;
        }

      g_debug (
        "new pos %s (%d)", new_at->port_id.label,
        new_at->index);
    }
}

/**
 * Updates the Track position of the Automatable's
 * and AutomationTrack's.
 *
 * @param track The Track to update to.
 */
void
automation_tracklist_update_track_name_hash (
  AutomationTracklist * self,
  Track *               track)
{
  self->track = track;
  unsigned int track_name_hash = track_get_name_hash (track);
  for (int i = 0; i < self->num_ats; i++)
    {
      AutomationTrack * at = self->ats[i];
      at->port_id.track_name_hash = track_name_hash;
      if (at->port_id.owner_type == PORT_OWNER_TYPE_PLUGIN)
        {
          at->port_id.plugin_id.track_name_hash =
            track_name_hash;
        }
      for (int j = 0; j < at->num_regions; j++)
        {
          ZRegion * region = at->regions[j];
          region->id.track_name_hash = track_name_hash;
          region_update_identifier (region);
        }
    }
}

/**
 * Unselects all arranger objects.
 */
void
automation_tracklist_unselect_all (AutomationTracklist * self)
{
  for (int i = self->num_ats - 1; i >= 0; i--)
    {
      AutomationTrack * at = self->ats[i];
      automation_track_unselect_all (at);
    }
}

/**
 * Removes all arranger objects recursively.
 */
void
automation_tracklist_clear (AutomationTracklist * self)
{
  for (int i = self->num_ats - 1; i >= 0; i--)
    {
      AutomationTrack * at = self->ats[i];
      automation_track_clear (at);
    }
}

/**
 * Clones the automation tracklist elements from
 * src to dest.
 */
void
automation_tracklist_clone (
  AutomationTracklist * src,
  AutomationTracklist * dest)
{
  dest->ats_size = (size_t) src->num_ats;
  dest->num_ats = src->num_ats;
  dest->ats = object_new_n (dest->ats_size, AutomationTrack *);

  for (int i = 0; i < src->num_ats; i++)
    {
      dest->ats[i] = automation_track_clone (src->ats[i]);
    }

  /* TODO create same automation lanes */
}

/**
 * Updates the frames of each position in each child
 * of the automation tracklist recursively.
 *
 * @param from_ticks Whether to update the
 *   positions based on ticks (true) or frames
 *   (false).
 */
void
automation_tracklist_update_positions (
  AutomationTracklist * self,
  bool                  from_ticks,
  bool                  bpm_change)
{
  for (int i = 0; i < self->num_ats; i++)
    {
      automation_track_update_positions (
        self->ats[i], from_ticks, bpm_change);
    }
}

/**
 * Gets the currently visible AutomationTrack's
 * (regardless of whether the automation tracklist
 * is visible in the UI or not.
 */
void
automation_tracklist_get_visible_tracks (
  AutomationTracklist * self,
  AutomationTrack **    visible_tracks,
  int *                 num_visible)
{
  *num_visible = 0;
  AutomationTrack * at;
  for (int i = 0; i < self->num_ats; i++)
    {
      at = self->ats[i];
      if (at->created && at->visible)
        {
          if (visible_tracks)
            {
              array_append (visible_tracks, *num_visible, at);
            }
          else
            {
              (*num_visible)++;
            }
        }
    }
}

/**
 * Returns the AutomationTrack corresponding to the
 * given Port.
 */
AutomationTrack *
automation_tracklist_get_at_from_port (
  AutomationTracklist * self,
  Port *                port)
{
  AutomationTrack * at;
  for (int i = 0; i < self->num_ats; i++)
    {
      at = self->ats[i];
      Port * at_port =
        port_find_from_identifier (&at->port_id);
      if (at_port == port)
        {
          return at;
        }
    }

  g_return_val_if_reached (NULL);
}

/**
 * Used when the add button is added and a new
 * automation track is requested to be shown.
 *
 * Marks the first invisible automation track as
 * visible, or marks an uncreated one as created
 * if all invisible ones are visible, and returns
 * it.
 */
AutomationTrack *
automation_tracklist_get_first_invisible_at (
  AutomationTracklist * self)
{
  /* prioritize automation tracks with existing
   * lanes */
  AutomationTrack * at;
  int               i;
  for (i = 0; i < self->num_ats; i++)
    {
      at = self->ats[i];
      if (at->created && !at->visible)
        {
          return at;
        }
    }

  for (i = 0; i < self->num_ats; i++)
    {
      at = self->ats[i];
      if (!at->created)
        {
          return at;
        }
    }

  /* all visible */
  return NULL;
}

/**
 * Removes the AutomationTrack from the
 * AutomationTracklist, optionally freeing it.
 */
void
automation_tracklist_remove_at (
  AutomationTracklist * self,
  AutomationTrack *     at,
  bool                  free_at,
  bool                  fire_events)
{
  int deleted_idx = 0;

#if 0
  g_debug (
    "[track %d atl] removing automation track at: "
    "%d '%s'",
    self->track_pos, deleted_idx, at->port_id.label);
#endif

  if (free_at)
    {
      /* this needs to be called before removing the
       * automation track in case the region is
       * referenced elsewhere (e.g., clip editor) */
      automation_track_clear (at);
    }

  array_delete_return_pos (
    self->ats, self->num_ats, at, deleted_idx);

  /* move automation track regions for automation
   * tracks after the deleted one*/
  for (int j = deleted_idx; j < self->num_ats; j++)
    {
      AutomationTrack * cur_at = self->ats[j];
      cur_at->index = j;
      for (int i = 0; i < cur_at->num_regions; i++)
        {
          ZRegion * region = cur_at->regions[i];
          region_set_automation_track (region, cur_at);
        }
    }

  /* if the deleted at was the last visible/created
   * at, make the next one visible */
  int num_visible;
  automation_tracklist_get_visible_tracks (
    self, NULL, &num_visible);
  if (num_visible == 0)
    {
      AutomationTrack * first_invisible_at =
        automation_tracklist_get_first_invisible_at (self);
      if (first_invisible_at)
        {
          if (!first_invisible_at->created)
            first_invisible_at->created = 1;
          first_invisible_at->visible = 1;

          if (fire_events)
            {
              EVENTS_PUSH (
                ET_AUTOMATION_TRACK_ADDED, first_invisible_at);
            }
        }
    }

  if (free_at)
    {
      automation_track_free (at);
    }

  if (fire_events)
    {
      EVENTS_PUSH (ET_AUTOMATION_TRACKLIST_AT_REMOVED, self);
    }
}

/**
 * Prints info about all the automation tracks.
 *
 * Used for debugging.
 */
void
automation_tracklist_print_ats (AutomationTracklist * self)
{
  GString * g_str = g_string_new (NULL);

  g_string_append_printf (
    g_str, "Automation tracklist (track '%s')\n",
    self->track->name);

  for (int i = 0; i < self->num_ats; i++)
    {
      AutomationTrack * at = self->ats[i];

      g_string_append_printf (
        g_str, "[%d] '%s' (sym '%s')\n", i, at->port_id.label,
        at->port_id.sym);
    }
  g_string_erase (g_str, (gssize) (g_str->len - 1), -1);

  char * str = g_string_free (g_str, false);
  g_message ("%s", str);
  g_free (str);
}

/**
 * Returns the number of visible AutomationTrack's.
 */
int
automation_tracklist_get_num_visible (
  AutomationTracklist * self)
{
  AutomationTrack * at;
  int               i;
  int               count = 0;
  for (i = 0; i < self->num_ats; i++)
    {
      at = self->ats[i];
      if (at->created && at->visible)
        {
          count++;
        }
    }

  return count;
}

/**
 * Verifies the identifiers on a live automation
 * tracklist (in the project, not a clone).
 *
 * @return True if pass.
 */
bool
automation_tracklist_validate (AutomationTracklist * self)
{
  g_return_val_if_fail (
    self->schema_version == AUTOMATION_TRACKLIST_SCHEMA_VERSION,
    false);
  g_return_val_if_fail (
    IS_TRACK_AND_NONNULL (self->track), false);

  unsigned int track_name_hash =
    track_get_name_hash (self->track);
  for (int i = 0; i < self->num_ats; i++)
    {
      AutomationTrack * at = self->ats[i];
      g_return_val_if_fail (
        at->port_id.track_name_hash == track_name_hash
          && at->index == i,
        false);
      automation_track_validate (at);
    }

  return true;
}

/**
 * Counts the total number of regions in the
 * automation tracklist.
 */
int
automation_tracklist_get_num_regions (
  AutomationTracklist * self)
{
  int count = 0;
  for (int i = 0; i < self->num_ats; i++)
    {
      AutomationTrack * at = self->ats[i];
      count += at->num_regions;
    }
  return count;
}

void
automation_tracklist_set_caches (
  AutomationTracklist * self,
  CacheTypes            types)
{
  Track * track = automation_tracklist_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  if (track_is_auditioner (track))
    return;

  if (types & CACHE_TYPE_AUTOMATION_LANE_RECORD_MODES)
    {
      self->ats_in_record_mode = g_realloc_n (
        self->ats_in_record_mode, (size_t) self->num_ats,
        sizeof (AutomationTrack *));
      self->num_ats_in_record_mode = 0;
    }

  for (int i = 0; i < self->num_ats; i++)
    {
      AutomationTrack * at = self->ats[i];
      automation_track_set_caches (at, types);

      if (
        types & CACHE_TYPE_AUTOMATION_LANE_RECORD_MODES
        && at->automation_mode == AUTOMATION_MODE_RECORD)
        {
          array_append (
            self->ats_in_record_mode,
            self->num_ats_in_record_mode, at);
        }
    }
}

void
automation_tracklist_print_regions (AutomationTracklist * self)
{
  Track * track = automation_tracklist_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  GString * str = g_string_new (NULL);
  g_string_append_printf (
    str,
    "Automation regions for track %s "
    "(total automation tracks %d):",
    track->name, self->num_ats);
  for (int i = 0; i < self->num_ats; i++)
    {
      AutomationTrack * at = self->ats[i];
      if (at->num_regions == 0)
        continue;

      g_string_append_printf (
        str, "\n  [%d] port '%s': %d regions", i,
        at->port_id.label, at->num_regions);
    }
  char * tmp = g_string_free (str, false);
  g_message ("%s", tmp);
  g_free (tmp);
}

void
automation_tracklist_free_members (AutomationTracklist * self)
{
  int size = self->num_ats;
  self->num_ats = 0;
  for (int i = 0; i < size; i++)
    {
      object_free_w_func_and_null (
        automation_track_free, self->ats[i]);
    }

  object_zero_and_free (self->ats);
  object_zero_and_free (self->ats_in_record_mode);
}
