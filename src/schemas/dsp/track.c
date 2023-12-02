// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/track.h"
#include "utils/objects.h"

#include "schemas/dsp/track.h"

Track *
track_upgrade_from_v1 (Track_v1 * old)
{
  if (!old)
    return NULL;

  Track * self = object_new (Track);

  self->schema_version = TRACK_SCHEMA_VERSION;
  self->name = old->name;
  self->icon_name = old->icon_name;
  self->type = (TrackType) old->type;
  self->pos = old->pos;
  self->lanes_visible = old->lanes_visible;
  self->automation_visible = old->automation_visible;
  self->visible = old->visible;
  self->main_height = old->main_height;
  self->passthrough_midi_input = old->passthrough_midi_input;
  self->recording = port_upgrade_from_v1 (old->recording);
  self->enabled = old->enabled;
  self->color = old->color;
  self->num_lanes = old->num_lanes;
  self->lanes = g_malloc_n ((size_t) self->num_lanes, sizeof (TrackLane *));
  for (int i = 0; i < self->num_lanes; i++)
    {
      self->lanes[i] = track_lane_upgrade_from_v1 (old->lanes[i]);
    }
  self->num_chord_regions = old->num_chord_regions;
  self->chord_regions =
    g_malloc_n ((size_t) self->num_chord_regions, sizeof (ZRegion *));
  for (int i = 0; i < self->num_chord_regions; i++)
    {
      self->chord_regions[i] = region_upgrade_from_v1 (old->chord_regions[i]);
    }
  self->num_scales = old->num_scales;
  self->scales = g_malloc_n ((size_t) self->num_scales, sizeof (ScaleObject *));
  for (int i = 0; i < self->num_scales; i++)
    {
      self->scales[i] = scale_object_upgrade_from_v1 (old->scales[i]);
    }
  self->num_markers = old->num_markers;
  self->markers = g_malloc_n ((size_t) self->num_markers, sizeof (Marker *));
  for (int i = 0; i < self->num_markers; i++)
    {
      self->markers[i] = marker_upgrade_from_v1 (old->markers[i]);
    }
  self->channel = channel_upgrade_from_v1 (old->channel);
  self->bpm_port = port_upgrade_from_v1 (old->bpm_port);
  self->beats_per_bar_port = port_upgrade_from_v1 (old->beats_per_bar_port);
  self->beat_unit_port = port_upgrade_from_v1 (old->beat_unit_port);
  self->num_modulators = old->num_modulators;
  self->modulators =
    g_malloc_n ((size_t) old->num_modulators, sizeof (Plugin *));
  for (int i = 0; i < self->num_modulators; i++)
    {
      self->modulators[i] = plugin_upgrade_from_v1 (old->modulators[i]);
    }
  self->num_modulator_macros = old->num_modulator_macros;
  for (int i = 0; i < self->num_modulator_macros; i++)
    {
      self->modulator_macros[i] =
        modulator_macro_processor_upgrade_from_v1 (old->modulator_macros[i]);
    }
  self->num_visible_modulator_macros = old->num_visible_modulator_macros;
  self->processor = track_processor_upgrade_from_v1 (old->processor);
  AutomationTracklist * atl =
    automation_tracklist_upgrade_from_v1 (&old->automation_tracklist);
  self->automation_tracklist = *atl;
  self->in_signal_type = (PortType) old->in_signal_type;
  self->out_signal_type = (PortType) old->out_signal_type;
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

Track *
track_upgrade_from_v2 (Track_v2 * old)
{
  if (!old)
    return NULL;

  Track * self = object_new (Track);

  self->schema_version = TRACK_SCHEMA_VERSION;
  self->name = old->name;
  self->icon_name = old->icon_name;
  self->type = (TrackType) old->type;
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
  self->lanes = g_malloc_n ((size_t) self->num_lanes, sizeof (TrackLane *));
  for (int i = 0; i < self->num_lanes; i++)
    {
      self->lanes[i] = old->lanes[i];
    }
  self->num_chord_regions = old->num_chord_regions;
  self->chord_regions =
    g_malloc_n ((size_t) self->num_chord_regions, sizeof (ZRegion *));
  for (int i = 0; i < self->num_chord_regions; i++)
    {
      self->chord_regions[i] = old->chord_regions[i];
    }
  self->num_scales = old->num_scales;
  self->scales = g_malloc_n ((size_t) self->num_scales, sizeof (ScaleObject *));
  for (int i = 0; i < self->num_scales; i++)
    {
      self->scales[i] = old->scales[i];
    }
  self->num_markers = old->num_markers;
  self->markers = g_malloc_n ((size_t) self->num_markers, sizeof (Marker *));
  for (int i = 0; i < self->num_markers; i++)
    {
      self->markers[i] = old->markers[i];
    }
  self->channel = channel_upgrade_from_v2 (old->channel);
  self->bpm_port = old->bpm_port;
  self->beats_per_bar_port = old->beats_per_bar_port;
  self->beat_unit_port = old->beat_unit_port;
  self->num_modulators = old->num_modulators;
  self->modulators =
    g_malloc_n ((size_t) old->num_modulators, sizeof (Plugin *));
  for (int i = 0; i < self->num_modulators; i++)
    {
      self->modulators[i] = plugin_upgrade_from_v2 (old->modulators[i]);
    }
  self->num_modulator_macros = old->num_modulator_macros;
  for (int i = 0; i < self->num_modulator_macros; i++)
    {
      self->modulator_macros[i] =
        old->modulator_macros[i];
    }
  self->num_visible_modulator_macros = old->num_visible_modulator_macros;
  self->processor = old->processor;
  AutomationTracklist * atl =
    &old->automation_tracklist;
  self->automation_tracklist = *atl;
  self->in_signal_type = (PortType) old->in_signal_type;
  self->out_signal_type = (PortType) old->out_signal_type;
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
