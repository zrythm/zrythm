// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/cyaml_schemas/dsp/channel.h"

#include "utils/objects.h"

Channel_v2 *
channel_upgrade_from_v1 (Channel_v1 * old)
{
  if (!old)
    return NULL;

  Channel_v2 * self = object_new (Channel_v2);

  self->schema_version = 2;
  for (int i = 0; i < 9; i++)
    {
      self->midi_fx[i] = old->midi_fx[i];
      self->inserts[i] = old->inserts[i];
      self->sends[i] = old->sends[i];
    }
  self->instrument = old->instrument;
  self->prefader = fader_upgrade_from_v1 (old->prefader);
  self->fader = fader_upgrade_from_v1 (old->fader);
  self->midi_out = old->midi_out;
  self->stereo_out = old->stereo_out;
  self->has_output = old->has_output;
  self->output_name_hash = old->output_name_hash;
  self->track_pos = old->track_pos;
  for (int i = 0; i < old->num_ext_midi_ins; i++)
    {
      self->ext_midi_ins[i] = old->ext_midi_ins[i];
    }
  for (int i = 0; i < old->num_ext_stereo_l_ins; i++)
    {
      self->ext_stereo_l_ins[i] = old->ext_stereo_l_ins[i];
    }
  for (int i = 0; i < old->num_ext_stereo_r_ins; i++)
    {
      self->ext_stereo_r_ins[i] = old->ext_stereo_r_ins[i];
    }
  self->num_ext_midi_ins = old->num_ext_midi_ins;
  self->all_midi_ins = old->all_midi_ins;
  memcpy (self->midi_channels, old->midi_channels, sizeof (old->midi_channels));
  self->all_midi_channels = old->all_midi_channels;
  self->num_ext_stereo_l_ins = old->num_ext_stereo_l_ins;
  self->all_stereo_l_ins = old->all_stereo_l_ins;
  self->num_ext_stereo_r_ins = old->num_ext_stereo_r_ins;
  self->all_stereo_r_ins = old->all_stereo_r_ins;
  self->width = old->width;

  return self;
}
