/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Velocities for MidiNote's.
 */

#ifndef __AUDIO_VELOCITY_H__
#define __AUDIO_VELOCITY_H__

#include <stdint.h>

#include "audio/region_identifier.h"
#include "gui/backend/arranger_object.h"

#include <cyaml/cyaml.h>

typedef struct MidiNote        MidiNote;
typedef struct _VelocityWidget VelocityWidget;

/**
 * @addtogroup audio
 *
 * @{
 */

#define VELOCITY_SCHEMA_VERSION 1

#define velocity_is_selected(r) \
  arranger_object_is_selected ( \
    (ArrangerObject *) r)

/**
 * Default velocity.
 */
#define VELOCITY_DEFAULT 90

/**
 * The MidiNote velocity.
 */
typedef struct Velocity
{
  /** Base struct. */
  ArrangerObject base;

  int schema_version;

  /** Velocity value (0-127). */
  uint8_t vel;

  /** Velocity at drag begin - used for ramp
   * actions only. */
  uint8_t vel_at_start;

  /** Pointer back to the MIDI note. */
  MidiNote * midi_note;
} Velocity;

static const cyaml_schema_field_t
  velocity_fields_schema[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      Velocity,
      base,
      arranger_object_fields_schema),
    YAML_FIELD_INT (Velocity, schema_version),
    YAML_FIELD_UINT (Velocity, vel), CYAML_FIELD_END
  };

static const cyaml_schema_value_t velocity_schema = {
  YAML_VALUE_PTR (Velocity, velocity_fields_schema),
};

/**
 * Creates a new Velocity with the given value.
 */
Velocity *
velocity_new (
  MidiNote *    midi_note,
  const uint8_t vel);

/**
 * Sets the MidiNote the Velocity belongs to.
 */
void
velocity_set_midi_note (
  Velocity * velocity,
  MidiNote * midi_note);

/**
 * Returns 1 if the Velocity's match, 0 if not.
 */
int
velocity_is_equal (Velocity * src, Velocity * dest);

/**
 * Changes the Velocity by the given amount of
 * values (delta).
 */
void
velocity_shift (Velocity * self, const int delta);

/**
 * Sets the velocity to the given value.
 *
 * The given value may exceed the bounds 0-127,
 * and will be clamped.
 */
void
velocity_set_val (Velocity * self, const int val);

/**
 * Returns the owner MidiNote.
 */
MidiNote *
velocity_get_midi_note (
  const Velocity * const self);

const char *
velocity_setting_enum_to_str (guint index);

guint
velocity_setting_str_to_enum (const char * str);

/**
 * @}
 */

#endif
