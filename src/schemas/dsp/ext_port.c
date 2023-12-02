// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/ext_port.h"
#include "utils/objects.h"

#include "schemas/dsp/ext_port.h"

ExtPort *
ext_port_create_from_v1 (ExtPort_v1 * old)
{
  if (!old)
    return NULL;

  ExtPort * self = object_new (ExtPort);

  self->schema_version = EXT_PORT_SCHEMA_VERSION;
  self->full_name = old->full_name;
  self->short_name = old->short_name;
  self->alias1 = old->alias1;
  self->alias2 = old->alias2;
  self->rtaudio_dev_name = old->rtaudio_dev_name;
  self->num_aliases = old->num_aliases;
  self->is_midi = old->is_midi;
  self->type = (ExtPortType) old->type;
  self->rtaudio_channel_idx = old->rtaudio_channel_idx;

  return self;
}
