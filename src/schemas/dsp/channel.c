// SPDX-FileCopyrightText: © 2022-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/channel.h"
#include "utils/objects.h"

#include "schemas/dsp/channel.h"

Channel *
channel_upgrade_from_v1 (Channel_v1 * old)
{
  if (!old)
    return NULL;

  Channel * self = object_new (Channel);

  self->schema_version = CHANNEL_SCHEMA_VERSION;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      self->midi_fx[i] = plugin_upgrade_from_v1 (old->midi_fx[i]);
      self->inserts[i] = plugin_upgrade_from_v1 (old->inserts[i]);
      self->sends[i] = channel_send_upgrade_from_v1 (old->sends[i]);
    }
  self->instrument = plugin_upgrade_from_v1 (old->instrument);
  self->prefader = fader_upgrade_from_v1 (old->prefader);
  self->fader = fader_upgrade_from_v1 (old->fader);
  self->midi_out = port_create_from_v1 (old->midi_out);
  self->stereo_out = stereo_ports_create_from_v1 (old->stereo_out);
  self->has_output = old->has_output;
  self->output_name_hash = old->output_name_hash;
  self->track_pos = old->track_pos;
  for (int i = 0; i < EXT_PORTS_MAX; i++)
    {
      self->ext_midi_ins[i] = ext_port_create_from_v1 (old->ext_midi_ins[i]);
      self->ext_stereo_l_ins[i] =
        ext_port_create_from_v1 (old->ext_stereo_l_ins[i]);
      self->ext_stereo_r_ins[i] =
        ext_port_create_from_v1 (old->ext_stereo_r_ins[i]);
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

Channel *
channel_upgrade_from_v2 (Channel_v2 * old)
{
  if (!old)
    return NULL;

  Channel * self = object_new (Channel);

  self->schema_version = CHANNEL_SCHEMA_VERSION;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      self->midi_fx[i] = plugin_upgrade_from_v2 (old->midi_fx[i]);
      self->inserts[i] = plugin_upgrade_from_v2 (old->inserts[i]);
      self->sends[i] = channel_send_create_from_v1 (old->sends[i]);
    }
  self->instrument = plugin_upgrade_from_v1 (old->instrument);
  self->prefader = fader_upgrade_from_v1 (old->prefader);
  self->fader = fader_upgrade_from_v1 (old->fader);
  self->midi_out = port_create_from_v1 (old->midi_out);
  self->stereo_out = stereo_ports_create_from_v1 (old->stereo_out);
  self->has_output = old->has_output;
  self->output_name_hash = old->output_name_hash;
  self->track_pos = old->track_pos;
  for (int i = 0; i < EXT_PORTS_MAX; i++)
    {
      self->ext_midi_ins[i] = ext_port_create_from_v1 (old->ext_midi_ins[i]);
      self->ext_stereo_l_ins[i] =
        ext_port_create_from_v1 (old->ext_stereo_l_ins[i]);
      self->ext_stereo_r_ins[i] =
        ext_port_create_from_v1 (old->ext_stereo_r_ins[i]);
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
