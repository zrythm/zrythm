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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/backend/events.h"
#include "gui/backend/mixer_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

void
mixer_selections_init_loaded (
  MixerSelections * ms)
{
}

/**
 * Returns if there are any selections.
 */
int
mixer_selections_has_any (
  MixerSelections * ms)
{
  return ms->num_slots;
}

/**
 * Gets highest slot in the selections.
 */
int
mixer_selections_get_highest_slot (
  MixerSelections * ms)
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
mixer_selections_get_lowest_slot (
  MixerSelections * ms)
{
  int max = -1;

  for (int i = 0; i < ms->num_slots; i++)
    {
      if (ms->slots[i] < max)
        max = ms->slots[i];
    }

  g_warn_if_fail (max > -1);

  return max;
}

/**
 * Get current Track.
 */
Track *
mixer_selections_get_track (
  MixerSelections * self)
{
  if (!self->has_any)
    return NULL;

  g_return_val_if_fail (
    self &&
    self->track_pos < TRACKLIST->num_tracks, NULL);
  Track * track =
    TRACKLIST->tracks[self->track_pos];
  g_return_val_if_fail (track, NULL);
  return track;
}

/**
 * Adds a slot to the selections.
 *
 * The selections can only be from one channel.
 *
 * @param ch The channel.
 * @param slot The slot to add to the selections.
 */
void
mixer_selections_add_slot (
  MixerSelections * ms,
  Channel *         ch,
  int               slot)
{
  if (!ms->has_any ||
      ch->track_pos != ms->track_pos)
    {
      mixer_selections_clear (
        ms, F_NO_PUBLISH_EVENTS);
      ms->track_pos = ch->track_pos;
    }
  ms->has_any = 1;

  if (array_contains_int (
        ms->slots, ms->num_slots, slot))
    return;

  array_double_append (
    ms->slots, ms->plugins, ms->num_slots,
    slot, ch->plugins[slot]);

  EVENTS_PUSH (
    ET_MIXER_SELECTIONS_CHANGED,
    ch->plugins[slot]);
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
  int               publish_events)
{
  g_message ("removing slot %d", slot);
  array_delete_primitive (
    ms->slots, ms->num_slots, slot);

  if (ms->num_slots == 0)
    {
      ms->has_any = 0;
      ms->track_pos = -1;
    }

  if (ZRYTHM_HAVE_UI && publish_events)
    {
      EVENTS_PUSH (ET_MIXER_SELECTIONS_CHANGED,
                   NULL);
    }
}


/**
 * Returns if the slot is selected or not.
 */
int
mixer_selections_contains_slot (
  MixerSelections * ms,
  int               slot)
{
  for (int i = 0; i < ms->num_slots; i++)
    if (ms->slots[i] == slot)
      return 1;

  return 0;
}

/**
 * Returns if the plugin is selected or not.
 */
int
mixer_selections_contains_plugin (
  MixerSelections * ms,
  Plugin *          pl)
{
  if (ms->track_pos != pl->id.track_pos)
    return 0;

  for (int i = 0; i < ms->num_slots; i++)
    {
      if (ms->slots[i] == pl->id.slot)
        return 1;
    }

  return 0;
}

/**
 * Clears selections.
 */
void
mixer_selections_clear (
  MixerSelections * ms,
  const int         pub_events)
{
  int i;

  for (i = ms->num_slots - 1; i >= 0; i--)
    {
      mixer_selections_remove_slot (
        ms, ms->slots[i], 0);
    }

  if (pub_events)
    {
      EVENTS_PUSH (ET_MIXER_SELECTIONS_CHANGED,
                   NULL);
    }

  g_message ("cleared mixer selections");
}

/**
 * Clone the struct for copying, undoing, etc.
 */
MixerSelections *
mixer_selections_clone (
  MixerSelections * src)
{
  MixerSelections * ms =
    calloc (1, sizeof (MixerSelections));

  int i;

  for (i = 0; i < src->num_slots; i++)
    {
      Plugin * pl =
        TRACKLIST->tracks[src->track_pos]->
          channel->plugins[src->slots[i]];
      g_return_val_if_fail (pl, NULL);
      ms->plugins[i] = plugin_clone (pl);
      ms->slots[i] = src->slots[i];
    }

  ms->num_slots = src->num_slots;
  ms->has_any = src->has_any;
  ms->track_pos = src->track_pos;

  return ms;
}

/**
 * Paste the selections starting at the slot in the
 * given channel.
 */
void
mixer_selections_paste_to_slot (
  MixerSelections * ts,
  Channel *         ch,
  int               slot)
{
  g_warn_if_reached ();

  /* TODO */
}

void
mixer_selections_free (MixerSelections * self)
{
  free (self);
}

SERIALIZE_SRC (
  MixerSelections, mixer_selections)
DESERIALIZE_SRC (
  MixerSelections, mixer_selections)
PRINT_YAML_SRC (
  MixerSelections, mixer_selections)

