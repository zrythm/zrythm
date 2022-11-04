// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/channel_send.h"
#include "utils/objects.h"
#include "schemas/audio/channel_send.h"

ChannelSend *
channel_send_upgrade_from_v1 (
  ChannelSend_v1 * old)
{
  if (!old)
    return NULL;

  ChannelSend * self = object_new (ChannelSend);

  self->schema_version = CHANNEL_SEND_SCHEMA_VERSION;
  self->slot = old->slot;
  self->stereo_in = stereo_ports_upgrade_from_v1 (old->stereo_in);
  self->midi_in = port_upgrade_from_v1 (old->midi_in);
  self->stereo_out = stereo_ports_upgrade_from_v1 (old->stereo_out);
  self->midi_out = port_upgrade_from_v1 (old->midi_out);
  self->amount = port_upgrade_from_v1 (old->amount);
  self->enabled = port_upgrade_from_v1 (old->enabled);
  self->is_sidechain = old->is_sidechain;
  self->track_name_hash = old->track_name_hash;

  return self;
}
