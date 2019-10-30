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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/midi_mapping.h"

/**
 * Initializes the MidiMappings after a Project
 * is loaded.
 */
void
midi_mappings_init_loaded (
  MidiMappings * self)
{
  /* TODO */
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
  MidiMapping * mapping =
    &self->mappings[self->num_mappings];
  memcpy (
    mapping->key, buf, sizeof (midi_byte_t) * 3);
  /*mapping->device_port =*/
    /*ext_port_clone (device_port);*/
  mapping->dest_id =
    dest_port->identifier;
  mapping->dest = dest_port;
  self->num_mappings++;

  char str[100];
  midi_ctrl_change_get_ch_and_description (
    buf, str);
  g_message (
    "bounded MIDI mapping from %s to %s",
    str, dest_port->identifier.label);
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
          port_set_control_value (
            mapping->dest,
            (float) buf[2] / 127.f);
        }
    }
}

/**
 * Returns a newly allocated MidiMappings.
 */
MidiMappings *
midi_mappings_new ()
{
  MidiMappings * self =
    calloc (1, sizeof (MidiMappings));

  return self;
}
