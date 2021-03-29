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

/**
 * \file
 *
 * Mapping MIDI CC to controls.
 */

#ifndef __AUDIO_MIDI_MAPPING_H__
#define __AUDIO_MIDI_MAPPING_H__

#include "audio/ext_port.h"
#include "audio/midi.h"
#include "audio/port.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define MIDI_MAPPINGS (PROJECT->midi_mappings)

/**
 * A mapping from a MIDI value to a destination.
 */
typedef struct MidiMapping
{
  int            schema_version;

  /** Raw MIDI signal. */
  midi_byte_t    key[3];

  /** The device that this connection will be mapped
   * for. */
  ExtPort *      device_port;

  /** Destination. */
  PortIdentifier dest_id;

  /** Destination pointer, for convenience. */
  Port *         dest;

  /** Whether this binding is enabled. */
  volatile int   enabled;
} MidiMapping;

/**
 * All MIDI mappings in Zrythm.
 */
typedef struct MidiMappings
{
  MidiMapping     mappings[2046];
  int             num_mappings;
} MidiMappings;

static const cyaml_schema_field_t
midi_mapping_fields_schema[] =
{
  YAML_FIELD_INT (
    MidiMapping, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY (
    MidiMapping, key,
    uint8_t_schema, 3),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    MidiMapping, device_port,
    ext_port_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    MidiMapping, dest_id,
    port_identifier_fields_schema),
  YAML_FIELD_INT (
    MidiMapping, enabled),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  midi_mapping_schema =
{
  YAML_VALUE_DEFAULT (
    MidiMapping, midi_mapping_fields_schema),
};

static const cyaml_schema_field_t
midi_mappings_fields_schema[] =
{
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    MidiMappings, mappings, midi_mapping_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
midi_mappings_schema =
{
  YAML_VALUE_PTR (
    MidiMappings, midi_mappings_fields_schema),
};

/**
 * Initializes the MidiMappings after a Project
 * is loaded.
 */
void
midi_mappings_init_loaded (
  MidiMappings * self);

/**
 * Returns a newly allocated MidiMappings.
 */
MidiMappings *
midi_mappings_new (void);

#define midi_mappings_bind( \
  self,buf,dev_port,dest_port,fire_events) \
  midi_mappings_bind_at ( \
    self, buf, dev_port, dest_port, \
    (self)->num_mappings, fire_events)

/**
 * Binds the CC represented by the given raw buffer
 * (must be size 3) to the given Port.
 *
 * @param idx Index to insert at.
 */
void
midi_mappings_bind_at (
  MidiMappings * self,
  midi_byte_t *  buf,
  ExtPort *      device_port,
  Port *         dest_port,
  int            idx,
  bool           fire_events);

/**
 * Unbinds the given binding.
 *
 * @note This must be called inside a port operation
 *   lock, such as inside an undoable action.
 */
void
midi_mappings_unbind (
  MidiMappings * self,
  int            idx,
  bool           fire_events);

void
midi_mapping_set_enabled (
  MidiMapping * self,
  bool          enabled);

int
midi_mapping_get_index (
  MidiMappings * self,
  MidiMapping *  mapping);

/**
 * Applies the given buffer to the matching ports.
 */
void
midi_mappings_apply (
  MidiMappings * self,
  midi_byte_t *  buf);

/**
 * Get MIDI mappings for the given port.
 *
 * @param size Size to set.
 *
 * @return a newly allocated array that must be
 * free'd with free().
 */
MidiMapping **
midi_mappings_get_for_port (
  MidiMappings * self,
  Port *         dest_port,
  int *          size);

void
midi_mappings_free (
  MidiMappings * self);

/**
 * @}
 */

#endif
