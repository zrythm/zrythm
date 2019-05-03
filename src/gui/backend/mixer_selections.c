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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/backend/events.h"
#include "gui/backend/mixer_selections.h"
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
  ms->ch = project_get_channel (ms->ch_id);
  g_warn_if_fail (ms->ch);
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
  if (ch != ms->ch)
    {
      mixer_selections_clear (ms);
      ms->ch_id = ch->id;
      ms->ch = ch;
    }

  if (array_contains_int (
        ms->slots,
        ms->num_slots,
        slot))
    return;

  array_double_append (
    ms->slots,
    ms->plugins,
    ms->num_slots,
    slot,
    ch->plugins[slot]);

  EVENTS_PUSH (ET_PLUGIN_SELECTION_CHANGED,
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
  array_delete (
    ms->slots,
    ms->num_slots,
    slot);

  if (ms->num_slots == 0)
    {
      ms->ch = NULL;
      ms->ch_id = -1;
    }

  if (publish_events)
    EVENTS_PUSH (ET_MIXER_SELECTIONS_CHANGED,
                 NULL);
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
  if (ms->ch != pl->channel)
    return 0;

  for (int i = 0; i < ms->num_slots; i++)
    if (ms->ch->plugins[ms->slots[i]] == pl)
      return 1;

  return 0;
}

/**
 * Clears selections.
 */
void
mixer_selections_clear (
  MixerSelections * ms)
{
  int i;

  for (i = ms->num_slots - 1; i >= 0; i--)
    {
      mixer_selections_remove_slot (
        ms, ms->slots[i], 0);
    }

  EVENTS_PUSH (ET_MIXER_SELECTIONS_CHANGED,
               NULL);

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
      ms->plugins[i] =
        plugin_clone (src->plugins[i]);
      ms->slots[i] = src->slots[i];
      g_return_val_if_fail (
        ms->plugins[i]->slot ==
          src->plugins[i]->slot, NULL);
    }

  ms->num_slots = src->num_slots;
  ms->ch_id = src->ch_id;
  ms->ch = channel_clone (src->ch);

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

