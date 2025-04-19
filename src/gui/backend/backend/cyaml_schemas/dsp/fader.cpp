// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/cyaml_schemas/dsp/fader.h"
#include "gui/backend/backend/cyaml_schemas/dsp/port.h"
#include "gui/backend/backend/cyaml_schemas/dsp/port_identifier.h"
#include "utils/objects.h"

Fader_v2 *
fader_upgrade_from_v1 (Fader_v1 * old)
{
  if (!old)
    return NULL;

  Fader_v2 * self = object_new (Fader_v2);

  self->schema_version = 2;
  self->type = old->type;
  self->volume = old->volume;
  self->amp = old->amp;
  self->phase = old->phase;
  self->balance = (old->balance);
  self->mute = (old->mute);
  self->solo = (old->solo);
  self->listen = (old->listen);
  self->mono_compat_enabled = (old->mono_compat_enabled);
  Port_v1 swap_phase = {};
  swap_phase.id = old->mono_compat_enabled->id;
  swap_phase.id.flow = Z_PORT_FLOW_INPUT_v1;
  swap_phase.id.label = const_cast<char *> (
    old->passthrough ? "Prefader Swap Phase" : "Fader Swap Phase");
  swap_phase.id.type = Z_PORT_TYPE_CONTROL_v1;
  swap_phase.id.sym = const_cast<char *> (
    old->passthrough ? "prefader_swap_phase" : "fader_swap_phase");
  swap_phase.id.flags2 |= Z_PORT_FLAG2_FADER_SWAP_PHASE_v1;
  swap_phase.id.flags |= Z_PORT_FLAG_TOGGLE_v1;
  swap_phase.minf = 0.f;
  swap_phase.maxf = 2.f;
  swap_phase.zerof = 0.f;
  auto      swap_phase_yaml = yaml_serialize (&swap_phase, &port_schema_v1);
  Port_v1 * swap_phase_v1 =
    (Port_v1 *) yaml_deserialize (swap_phase_yaml.c_str (), &port_schema_v1);
  self->swap_phase = swap_phase_v1;
  self->swap_phase->id.track_name_hash = self->amp->id.track_name_hash;
  self->swap_phase->id.owner_type = self->amp->id.owner_type;
  self->midi_in = (old->midi_in);
  self->midi_out = (old->midi_out);
  self->stereo_in = (old->stereo_in);
  self->stereo_out = (old->stereo_out);
  self->midi_mode = old->midi_mode;
  self->passthrough = old->passthrough;

  return self;
}
