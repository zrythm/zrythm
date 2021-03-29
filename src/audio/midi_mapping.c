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

#include "audio/control_port.h"
#include "audio/midi_mapping.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

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

static void
move_mapping (
  MidiMappings * self,
  int            dest_idx,
  int            src_idx)
{
  MidiMapping * src_mapping =
    &self->mappings[src_idx];
  MidiMapping * dest_mapping =
    &self->mappings[dest_idx];
  memcpy (
    dest_mapping->key, src_mapping->key,
    sizeof (midi_byte_t) * 3);
  dest_mapping->device_port =
    src_mapping->device_port;
  dest_mapping->dest_id = src_mapping->dest_id;
  dest_mapping->dest = src_mapping->dest;
  g_atomic_int_set (
    &dest_mapping->enabled,
    (guint)
    g_atomic_int_get (&src_mapping->enabled));
}

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
  bool           fire_events)
{
  g_return_if_fail (
    self && buf && dest_port);

  for (int i = self->num_mappings; i > idx; i--)
    {
      move_mapping (self, i, i - 1);
    }
  self->num_mappings++;

  MidiMapping * mapping = &self->mappings[idx];
  memcpy (
    mapping->key, buf, sizeof (midi_byte_t) * 3);
  if (device_port)
    {
      mapping->device_port =
        ext_port_clone (device_port);
    }
  mapping->dest_id = dest_port->id;
  mapping->dest = dest_port;
  g_atomic_int_set (
    &mapping->enabled, (guint) true);

  char str[100];
  midi_ctrl_change_get_ch_and_description (
    buf, str);
  g_message (
    "bounded MIDI mapping from %s to %s",
    str, dest_port->id.label);

  if (fire_events && ZRYTHM_HAVE_UI)
    {
      EVENTS_PUSH (ET_MIDI_BINDINGS_CHANGED, NULL);
    }
}

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
  bool           fire_events)
{
  g_return_if_fail (self && idx >= 0);

  MidiMapping * mapping_before =
    &self->mappings[idx];
  object_free_w_func_and_null (
    ext_port_free, mapping_before->device_port);

  for (int i = self->num_mappings - 2; i >= idx; i--)
    {
      move_mapping (self, i, i + 1);
    }
  self->num_mappings--;

  if (fire_events && ZRYTHM_HAVE_UI)
    {
      EVENTS_PUSH (ET_MIDI_BINDINGS_CHANGED, NULL);
    }
}

void
midi_mapping_set_enabled (
  MidiMapping * self,
  bool          enabled)
{
  g_atomic_int_set (
    &self->enabled, (guint) enabled);
}

int
midi_mapping_get_index (
  MidiMappings * self,
  MidiMapping *  mapping)
{
  for (int i = 0; i < self->num_mappings; i++)
    {
      if (&self->mappings[i] == mapping)
        return i;
    }
  g_return_val_if_reached (-1);
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

      if (g_atomic_int_get (&mapping->enabled) &&
          mapping->key[0] == buf[0] &&
          mapping->key[1] == buf[1])
        {
          g_return_if_fail (mapping->dest);

          Port * dest = mapping->dest;
          if (dest->id.type == TYPE_CONTROL)
            {
              float normalized_val =
                (float) buf[2] / 127.f;
              port_set_control_value (
                mapping->dest,
                control_port_normalized_val_to_real (
                  mapping->dest, normalized_val),
                false, true);
            }
          else if (dest->id.type == TYPE_EVENT)
            {
              if (dest->id.flags2 &
                    PORT_FLAG2_TRANSPORT_ROLL)
                {
                  transport_request_roll (TRANSPORT);
                }
              else if (dest->id.flags2 &
                    PORT_FLAG2_TRANSPORT_STOP)
                {
                  transport_request_pause (
                    TRANSPORT);
                }
              else if (dest->id.flags2 &
                    PORT_FLAG2_TRANSPORT_BACKWARD)
                {
                  transport_move_backward (
                    TRANSPORT);
                }
              else if (dest->id.flags2 &
                    PORT_FLAG2_TRANSPORT_FORWARD)
                {
                  transport_move_forward (TRANSPORT);
                }
              else if (dest->id.flags2 &
                    PORT_FLAG2_TRANSPORT_LOOP_TOGGLE)
                {
                  transport_set_loop (
                    TRANSPORT, !TRANSPORT->loop);
                }
              else if (dest->id.flags2 &
                    PORT_FLAG2_TRANSPORT_REC_TOGGLE)
                {
                  transport_set_recording (
                    TRANSPORT,
                    !TRANSPORT->recording,
                    F_PUBLISH_EVENTS);
                }
            }
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
            g_realloc (
              arr,
              (size_t) (*count + 1) *
                sizeof (MidiMapping *));
          array_append (
            arr, *count, mapping);
        }
    }
  return arr;
}
