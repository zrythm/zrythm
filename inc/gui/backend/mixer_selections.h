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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_BACKEND_MIXER_SELECTIONS_H__
#define __GUI_BACKEND_MIXER_SELECTIONS_H__

#include "audio/automation_point.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "utils/yaml.h"

#define MIXER_SELECTIONS \
  (&PROJECT->mixer_selections)

typedef struct Plugin Plugin;

/**
 * Selections to be used for the timeline's current
 * selections, copying, undoing, etc.
 */
typedef struct MixerSelections
{
  /** Slots selected. */
  int       slots[60];

  /** Cache. */
  Plugin *  plugins[60];

  int       num_slots;

  /** Channel selected. */
  int       track_pos;

  /** Cache. */
  Track * track;
} MixerSelections;

static const cyaml_schema_field_t
  mixer_selections_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "slots", CYAML_FLAG_DEFAULT,
    MixerSelections, slots,
    num_slots,
    &int_schema, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "track_pos", CYAML_FLAG_DEFAULT,
    MixerSelections, track_pos),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
mixer_selections_schema = {
  CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
    MixerSelections, mixer_selections_fields_schema),
};

void
mixer_selections_init_loaded (
  MixerSelections * ms);

/**
 * Clone the struct for copying, undoing, etc.
 */
MixerSelections *
mixer_selections_clone (
  MixerSelections * src);

/**
 * Returns if there are any selections.
 */
int
mixer_selections_has_any (
  MixerSelections * ms);

/**
 * Gets highest slot in the selections.
 */
int
mixer_selections_get_highest_slot (
  MixerSelections * ms);

/**
 * Gets lowest slot in the selections.
 */
int
mixer_selections_get_lowest_slot (
  MixerSelections * ms);

/**
 * Paste the selections starting at the slot in the
 * given channel.
 */
void
mixer_selections_paste_to_slot (
  MixerSelections * ts,
  Channel *         ch,
  int               slot);

/**
 * Returns if the slot is selected or not.
 */
int
mixer_selections_contains_slot (
  MixerSelections * ms,
  int               slot);

/**
 * Returns if the plugin is selected or not.
 */
int
mixer_selections_contains_plugin (
  MixerSelections * ms,
  Plugin *          pl);

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
  int               slot);

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
  int               publish_events);

/**
 * Clears selections.
 */
void
mixer_selections_clear (
  MixerSelections * ms);

void
mixer_selections_free (MixerSelections * self);

SERIALIZE_INC (MixerSelections, mixer_selections)
DESERIALIZE_INC (MixerSelections, mixer_selections)
PRINT_YAML_INC (MixerSelections, mixer_selections)

#endif
