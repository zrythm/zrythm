// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/fader.h"
#include "schemas/dsp/fader.h"
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
  Port * swap_phase = port_new_with_type (
    TYPE_CONTROL, FLOW_INPUT,
    old->passthrough ? "Prefader Swap Phase" : "Fader Swap Phase");
  swap_phase->id.sym =
    old->passthrough
      ? g_strdup ("prefader_swap_phase")
      : g_strdup ("fader_swap_phase");
  swap_phase->id.flags2 |= PORT_FLAG2_FADER_SWAP_PHASE;
  swap_phase->id.flags |= PORT_FLAG_TOGGLE;
  GError *  err = NULL;
  char *    swap_phase_yaml = yaml_serialize (swap_phase, &port_schema, &err);
  Port_v1 * swap_phase_v1 =
    yaml_deserialize (swap_phase_yaml, &port_schema_v1, &err);
  g_free (swap_phase_yaml);
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
