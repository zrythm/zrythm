// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/track_lane.h"
#include "utils/objects.h"

#include "schemas/audio/track_lane.h"

TrackLane *
track_lane_upgrade_from_v1 (TrackLane_v1 * old)
{
  if (!old)
    return NULL;

  TrackLane * self = object_new (TrackLane);

  self->schema_version = TRACK_LANE_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  UPDATE (pos);
  UPDATE (name);
  UPDATE (height);
  UPDATE (mute);
  UPDATE (solo);
  UPDATE (midi_ch);
  self->num_regions = old->num_regions;
  self->regions = g_malloc_n (
    (size_t) self->num_regions, sizeof (ZRegion *));
  for (int i = 0; i < self->num_regions; i++)
    {
      self->regions[i] =
        region_upgrade_from_v1 (old->regions[i]);
    }

  return self;
}
