// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Mapping MIDI CC to controls.
 */

#ifndef __AUDIO_MIDI_MAPPING_H__
#define __AUDIO_MIDI_MAPPING_H__

#include "dsp/ext_port.h"
#include "dsp/port.h"
#include "utils/midi.h"

typedef struct _WrappedObjectWithChangeSignal WrappedObjectWithChangeSignal;

/**
 * @addtogroup dsp
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
  int enabled;

  /** Used in Gtk. */
  WrappedObjectWithChangeSignal * gobj;
} MidiMapping;

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

#define midi_mappings_bind_device(self, buf, dev_port, dest_port, fire_events) \
  midi_mappings_bind_at ( \
    self, buf, dev_port, dest_port, (self)->num_mappings, fire_events)

#define midi_mappings_bind_track(self, buf, dest_port, fire_events) \
  midi_mappings_bind_at ( \
    self, buf, NULL, dest_port, (self)->num_mappings, fire_events)

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
midi_mappings_unbind (MidiMappings * self, int idx, bool fire_events);

MidiMapping *
midi_mapping_new (void);

void
midi_mapping_set_enabled (MidiMapping * self, bool enabled);

int
midi_mapping_get_index (MidiMappings * self, MidiMapping * mapping);

NONNULL MidiMapping *
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
 * @param arr Optional array to fill with the mappings.
 *
 * @return The number of results.
 */
int
midi_mappings_get_for_port (
  MidiMappings * self,
  Port *         dest_port,
  GPtrArray *    arr);

MidiMappings *
midi_mappings_clone (const MidiMappings * src);

void
midi_mappings_free (MidiMappings * self);

/**
 * @}
 */

#endif
