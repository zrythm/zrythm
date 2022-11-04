// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/track_processor.h"
#include "utils/objects.h"

#include "schemas/audio/track_processor.h"

TrackProcessor *
track_processor_upgrade_from_v1 (TrackProcessor_v1 * old)
{
  if (!old)
    return NULL;

  TrackProcessor * self = object_new (TrackProcessor);

  self->schema_version = TRACK_PROCESSOR_SCHEMA_VERSION;
  self->mono = port_upgrade_from_v1 (old->mono);
  self->input_gain = port_upgrade_from_v1 (old->input_gain);
  self->output_gain = port_upgrade_from_v1 (old->output_gain);
  self->midi_in = port_upgrade_from_v1 (old->midi_in);
  self->midi_out = port_upgrade_from_v1 (old->midi_out);
  self->piano_roll = port_upgrade_from_v1 (old->piano_roll);
  self->monitor_audio =
    port_upgrade_from_v1 (old->monitor_audio);
  self->stereo_in =
    stereo_ports_upgrade_from_v1 (old->stereo_in);
  self->stereo_out =
    stereo_ports_upgrade_from_v1 (old->stereo_out);
  for (int i = 0; i < 128 * 16; i++)
    {
      self->midi_cc[i] =
        port_upgrade_from_v1 (old->midi_cc[i]);
    }
  for (int i = 0; i < 16; i++)
    {
      self->pitch_bend[i] =
        port_upgrade_from_v1 (old->pitch_bend[i]);
      self->poly_key_pressure[i] =
        port_upgrade_from_v1 (old->poly_key_pressure[i]);
      self->channel_pressure[i] =
        port_upgrade_from_v1 (old->channel_pressure[i]);
    }

  return self;
}
