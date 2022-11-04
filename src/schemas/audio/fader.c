// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/fader.h"
#include "utils/objects.h"
#include "schemas/audio/fader.h"

Fader *
fader_upgrade_from_v1 (
  Fader_v1 * old)
{
  if (!old)
    return NULL;

  Fader * self = object_new (Fader);

  self->schema_version = FADER_SCHEMA_VERSION;
  self->type = (FaderType) old->type;
  self->volume = old->volume;
  self->amp = port_upgrade_from_v1 (old->amp);
  self->phase = old->phase;
  self->balance = port_upgrade_from_v1 (old->balance);
  self->mute = port_upgrade_from_v1 (old->mute);
  self->solo = port_upgrade_from_v1 (old->solo);
  self->listen = port_upgrade_from_v1 (old->listen);
  self->mono_compat_enabled = port_upgrade_from_v1 (old->mono_compat_enabled);
  self->swap_phase = fader_create_swap_phase_port (self, old->passthrough);
  self->swap_phase->id.track_name_hash = self->amp->id.track_name_hash;
  self->swap_phase->id.owner_type = self->amp->id.owner_type;
  self->midi_in = port_upgrade_from_v1 (old->midi_in);
  self->midi_out = port_upgrade_from_v1 (old->midi_out);
  self->stereo_in = stereo_ports_upgrade_from_v1 (old->stereo_in);
  self->stereo_out = stereo_ports_upgrade_from_v1 (old->stereo_out);
  self->midi_mode = (MidiFaderMode) old->midi_mode;
  self->passthrough = old->passthrough;

  return self;
}
