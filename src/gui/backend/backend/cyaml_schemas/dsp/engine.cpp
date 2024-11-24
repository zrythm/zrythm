// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/cyaml_schemas/dsp/engine.h"

#include "utils/objects.h"

AudioEngine_v2 *
engine_upgrade_from_v1 (AudioEngine_v1 * old)
{
  if (!old)
    return NULL;

  AudioEngine_v2 * self = object_new (AudioEngine_v2);

  self->schema_version = 2;
  self->transport_type = old->transport_type;
  self->sample_rate = old->sample_rate;
  self->frames_per_tick = old->frames_per_tick;
  self->monitor_out = (old->monitor_out);
  self->midi_editor_manual_press = (old->midi_editor_manual_press);
  self->midi_in = (old->midi_in);
  self->transport = (old->transport);
  self->pool = (old->pool);
  self->control_room = control_room_upgrade_from_v1 (old->control_room);
  self->sample_processor =
    sample_processor_upgrade_from_v1 (old->sample_processor);
  self->hw_in_processor = (old->hw_in_processor);
  self->hw_out_processor = (old->hw_out_processor);

  return self;
}
