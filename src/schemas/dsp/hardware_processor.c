// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/hardware_processor.h"
#include "utils/objects.h"

#include "schemas/dsp/hardware_processor.h"

HardwareProcessor *
hardware_processor_upgrade_from_v1 (HardwareProcessor_v1 * old)
{
  if (!old)
    return NULL;

  HardwareProcessor * self = object_new (HardwareProcessor);

  self->schema_version = HW_PROCESSOR_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name
#define UPDATE_EXT_PORT(name) \
  self->name = ext_port_upgrade_from_v1 (old->name)
#define UPDATE_PORT(name) \
  self->name = port_upgrade_from_v1 (old->name)

  UPDATE (is_input);
  UPDATE (num_ext_audio_ports);
  self->ext_audio_ports = g_malloc_n (
    (size_t) self->num_ext_audio_ports, sizeof (ExtPort *));
  for (int i = 0; i < self->num_ext_audio_ports; i++)
    {
      UPDATE_EXT_PORT (ext_audio_ports[i]);
    }
  UPDATE (num_ext_midi_ports);
  self->ext_midi_ports = g_malloc_n (
    (size_t) self->num_ext_midi_ports, sizeof (ExtPort *));
  for (int i = 0; i < self->num_ext_midi_ports; i++)
    {
      UPDATE_EXT_PORT (ext_midi_ports[i]);
    }
  UPDATE (num_audio_ports);
  self->audio_ports = g_malloc_n (
    (size_t) self->num_audio_ports, sizeof (Port *));
  for (int i = 0; i < self->num_audio_ports; i++)
    {
      UPDATE_PORT (audio_ports[i]);
    }
  UPDATE (num_midi_ports);
  self->midi_ports = g_malloc_n (
    (size_t) self->num_midi_ports, sizeof (Port *));
  for (int i = 0; i < self->num_midi_ports; i++)
    {
      UPDATE_PORT (midi_ports[i]);
    }

  return self;
}
