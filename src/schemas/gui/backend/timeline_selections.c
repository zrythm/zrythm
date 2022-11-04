// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/timeline_selections.h"
#include "utils/objects.h"

#include "schemas/gui/backend/timeline_selections.h"

TimelineSelections *
timeline_selections_upgrade_from_v1 (
  TimelineSelections_v1 * old)
{
  if (!old)
    return NULL;

  TimelineSelections * self = object_new (TimelineSelections);

  self->schema_version = TL_SELECTIONS_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  ArrangerSelections * base =
    arranger_selections_upgrade_from_v1 (&old->base);
  self->base = *base;
  self->num_regions = old->num_regions;
  self->regions = g_malloc_n (
    (size_t) self->num_regions, sizeof (ZRegion *));
  for (int i = 0; i < self->num_regions; i++)
    {
      self->regions[i] =
        region_upgrade_from_v1 (old->regions[i]);
    }
  self->num_scale_objects = old->num_scale_objects;
  self->scale_objects = g_malloc_n (
    (size_t) self->num_scale_objects, sizeof (ZRegion *));
  for (int i = 0; i < self->num_scale_objects; i++)
    {
      self->scale_objects[i] =
        scale_object_upgrade_from_v1 (old->scale_objects[i]);
    }
  self->num_markers = old->num_markers;
  self->markers = g_malloc_n (
    (size_t) self->num_markers, sizeof (ZRegion *));
  for (int i = 0; i < self->num_markers; i++)
    {
      self->markers[i] =
        marker_upgrade_from_v1 (old->markers[i]);
    }
  UPDATE (region_track_vis_index);
  UPDATE (chord_track_vis_index);
  UPDATE (marker_track_vis_index);

  return self;
}
