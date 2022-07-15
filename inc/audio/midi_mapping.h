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

/**
 * \file
 *
 * Mapping MIDI CC to controls.
 */

#ifndef __AUDIO_MIDI_MAPPING_H__
#define __AUDIO_MIDI_MAPPING_H__

#include "audio/ext_port.h"
#include "audio/port.h"
#include "utils/midi.h"

typedef struct _WrappedObjectWithChangeSignal
  WrappedObjectWithChangeSignal;

/**
 * @addtogroup audio
 *
 * @{
 */

#define MIDI_MAPPING_SCHEMA_VERSION 1
#define MIDI_MAPPINGS_SCHEMA_VERSION 1

#define MIDI_MAPPINGS (PROJECT->midi_mappings)

/**
 * A mapping from a MIDI value to a destination.
 */
typedef struct MidiMapping
{
  int schema_version;

  /** Raw MIDI signal. */
  midi_byte_t key[3];

  /** The device that this connection will be mapped
   * for. */
  ExtPort * device_port;

  /** Destination. */
  PortIdentifier dest_id;

  /**
   * Destination pointer, for convenience.
   *
   * @note This pointer is not owned by this
   *   instance.
   */
  Port * dest;

  /** Whether this binding is enabled. */
  volatile int enabled;

  /** Used in Gtk. */
  WrappedObjectWithChangeSignal * gobj;
} MidiMapping;

static const cyaml_schema_field_t midi_mapping_fields_schema[] = {
  YAML_FIELD_INT (MidiMapping, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY (
    MidiMapping,
    key,
    uint8_t_schema,
    3),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    MidiMapping,
    device_port,
    ext_port_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    MidiMapping,
    dest_id,
    port_identifier_fields_schema),
  YAML_FIELD_INT (MidiMapping, enabled),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t midi_mapping_schema = {
  YAML_VALUE_PTR (MidiMapping, midi_mapping_fields_schema),
};

#if 0
static const cyaml_schema_value_t
  midi_mapping_schema_default =
{
  YAML_VALUE_DEFAULT (
    MidiMapping, midi_mapping_fields_schema),
};
#endif

/**
 * All MIDI mappings in Zrythm.
 */
typedef struct MidiMappings
{
  int schema_version;

  MidiMapping ** mappings;
  size_t         mappings_size;
  int            num_mappings;
} MidiMappings;

static const cyaml_schema_field_t midi_mappings_fields_schema[] = {
  YAML_FIELD_INT (MidiMappings, schema_version),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    MidiMappings,
    mappings,
    midi_mapping_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t midi_mappings_schema = {
  YAML_VALUE_PTR (MidiMappings, midi_mappings_fields_schema),
};

/**
 * Initializes the MidiMappings after a Project
 * is loaded.
 */
void
midi_mappings_init_loaded (MidiMappings * self);

/**
 * Returns a newly allocated MidiMappings.
 */
MidiMappings *
midi_mappings_new (void);

#define midi_mappings_bind_device( \
  self, buf, dev_port, dest_port, fire_events) \
  midi_mappings_bind_at ( \
    self, buf, dev_port, dest_port, (self)->num_mappings, \
    fire_events)

#define midi_mappings_bind_track( \
  self, buf, dest_port, fire_events) \
  midi_mappings_bind_at ( \
    self, buf, NULL, dest_port, (self)->num_mappings, \
    fire_events)

/**
 * Binds the CC represented by the given raw buffer
 * (must be size 3) to the given Port.
 *
 * @param idx Index to insert at.
 * @param buf The buffer used for matching at [0] and
 *   [1].
 * @param device_port Device port, if custom mapping.
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

MidiMapping *
midi_mapping_new (void);

void
midi_mapping_set_enabled (MidiMapping * self, bool enabled);

int
midi_mapping_get_index (
  MidiMappings * self,
  MidiMapping *  mapping);

NONNULL
MidiMapping *
midi_mapping_clone (const MidiMapping * src);

void
midi_mapping_free (MidiMapping * self);

/**
 * Applies the events to the appropriate mapping.
 *
 * This is used only for TrackProcessor.cc_mappings.
 *
 * @note Must only be called while transport is
 *   recording.
 */
void
midi_mappings_apply_from_cc_events (
  MidiMappings * self,
  MidiEvents *   events,
  bool           queued);

/**
 * Applies the given buffer to the matching ports.
 */
void
midi_mappings_apply (MidiMappings * self, midi_byte_t * buf);

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

MidiMappings *
midi_mappings_clone (const MidiMappings * src);

void
midi_mappings_free (MidiMappings * self);

/**
 * @}
 */

#endif
