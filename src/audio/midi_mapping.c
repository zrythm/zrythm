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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/control_port.h"
#include "audio/midi_mapping.h"
#include "utils/arrays.h"
#include "utils/objects.h"

/**
 * Initializes the MidiMappings after a Project
 * is loaded.
 */
void
midi_mappings_init_loaded (
  MidiMappings * self)
{
  for (int i = 0; i < self->num_mappings; i++)
    {
      MidiMapping * mapping =  &self->mappings[i];
      mapping->dest =
        port_find_from_identifier (
          &mapping->dest_id);
    }
}

/**
 * Binds the CC represented by the given raw buffer
 * (must be size 3) to the given Port.
 */
void
midi_mappings_bind (
  MidiMappings * self,
  midi_byte_t *  buf,
  ExtPort *      device_port,
  Port *         dest_port)
{
  g_return_if_fail (
    self && buf && dest_port);
  MidiMapping * mapping =
    &self->mappings[self->num_mappings];
  memcpy (
    mapping->key, buf, sizeof (midi_byte_t) * 3);
  if (device_port)
    {
      mapping->device_port =
        ext_port_clone (device_port);
    }
  mapping->dest_id = dest_port->id;
  mapping->dest = dest_port;
  self->num_mappings++;

  char str[100];
  midi_ctrl_change_get_ch_and_description (
    buf, str);
  g_message (
    "bounded MIDI mapping from %s to %s",
    str, dest_port->id.label);
}

/**
 * Applies the given buffer to the matching ports.
 */
void
midi_mappings_apply (
  MidiMappings * self,
  midi_byte_t *  buf)
{
  for (int i = 0; i < self->num_mappings; i++)
    {
      MidiMapping * mapping = &self->mappings[i];

      if (mapping->key[0] == buf[0] &&
          mapping->key[1] == buf[1])
        {
          g_return_if_fail (mapping->dest);
          float normalized_val =
            (float) buf[2] / 127.f;
          port_set_control_value (
            mapping->dest,
            control_port_normalized_val_to_real (
              mapping->dest, normalized_val),
            false, true);
        }
    }
}

/**
 * Returns a newly allocated MidiMappings.
 */
MidiMappings *
midi_mappings_new ()
{
  MidiMappings * self = object_new (MidiMappings);

  return self;
}

void
midi_mappings_free (
  MidiMappings * self)
{
  for (int i = 0; i < self->num_mappings; i++)
    {
      object_free_w_func_and_null (
        ext_port_free,
        self->mappings[i].device_port);
    }

  object_zero_and_free (self);
}

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
  int *          count)
{
  g_return_val_if_fail (self && dest_port, NULL);

  MidiMapping ** arr = NULL;
  *count = 0;
  for (int i = 0; i < self->num_mappings; i++)
    {
      MidiMapping * mapping =
        &self->mappings[i];
      if (mapping->dest == dest_port)
        {
          arr =
            realloc (
              arr,
              (size_t) (*count + 1) *
                sizeof (MidiMapping *));
          array_append (
            arr, *count, mapping);
        }
    }
  return arr;
}
