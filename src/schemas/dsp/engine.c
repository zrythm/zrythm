// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "utils/objects.h"

#include "schemas/dsp/engine.h"

AudioEngine *
engine_upgrade_from_v1 (AudioEngine_v1 * old)
{
  if (!old)
    return NULL;

  AudioEngine * self = object_new (AudioEngine);

  self->schema_version = AUDIO_ENGINE_SCHEMA_VERSION;
  self->transport_type =
    (AudioEngineJackTransportType) old->transport_type;
  self->sample_rate = old->sample_rate;
  self->frames_per_tick = old->frames_per_tick;
  self->monitor_out =
    stereo_ports_upgrade_from_v1 (old->monitor_out);
  self->midi_editor_manual_press =
    port_upgrade_from_v1 (old->midi_editor_manual_press);
  self->midi_in = port_upgrade_from_v1 (old->midi_in);
  self->transport = transport_upgrade_from_v1 (old->transport);
  self->pool = audio_pool_upgrade_from_v1 (old->pool);
  self->control_room =
    control_room_upgrade_from_v1 (old->control_room);
  self->sample_processor =
    sample_processor_upgrade_from_v1 (old->sample_processor);
  self->hw_in_processor =
    hardware_processor_upgrade_from_v1 (old->hw_in_processor);
  self->hw_out_processor =
    hardware_processor_upgrade_from_v1 (old->hw_out_processor);

  return self;
}
