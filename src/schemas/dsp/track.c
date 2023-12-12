// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "schemas/dsp/track.h"
#include "utils/objects.h"

Track_v2 *
track_upgrade_from_v1 (Track_v1 * old)
{
  if (!old)
    return NULL;

  Track_v2 * self = object_new (Track_v2);

  self->schema_version = 2;
  self->name = old->name;
  self->icon_name = old->icon_name;
  self->type = old->type;
  self->pos = old->pos;
  self->lanes_visible = old->lanes_visible;
  self->automation_visible = old->automation_visible;
  self->visible = old->visible;
  self->main_height = old->main_height;
  self->passthrough_midi_input = old->passthrough_midi_input;
  self->recording = old->recording;
  self->enabled = old->enabled;
  self->color = old->color;
  self->num_lanes = old->num_lanes;
  self->lanes = old->lanes;
  self->num_chord_regions = old->num_chord_regions;
  self->chord_regions = old->chord_regions;
  self->num_scales = old->num_scales;
  self->scales = old->scales;
  self->num_markers = old->num_markers;
  self->markers = old->markers;
  self->channel = channel_upgrade_from_v1 (old->channel);
  self->bpm_port = old->bpm_port;
  self->beats_per_bar_port = old->beats_per_bar_port;
  self->beat_unit_port = old->beat_unit_port;
  self->num_modulators = old->num_modulators;
  self->modulators = old->modulators;
  self->num_modulator_macros = old->num_modulator_macros;
  for (int i = 0; i < old->num_modulator_macros; i++)
    {
      self->modulator_macros[i] = old->modulator_macros[i];
    }
  self->num_visible_modulator_macros = old->num_visible_modulator_macros;
  self->processor = (old->processor);
  self->automation_tracklist = old->automation_tracklist;
  self->in_signal_type = old->in_signal_type;
  self->out_signal_type = old->out_signal_type;
  self->midi_ch = old->midi_ch;
  self->comment = old->comment;
  self->num_children = old->num_children;
  self->children = old->children;
  self->frozen = old->frozen;
  self->pool_id = old->pool_id;
  self->size = old->size;
  self->folded = old->folded;
  self->record_set_automatically = old->record_set_automatically;
  self->drum_mode = old->drum_mode;

  return self;
}
