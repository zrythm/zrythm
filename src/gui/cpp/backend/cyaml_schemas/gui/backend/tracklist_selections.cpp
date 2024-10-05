// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

// #include "gui/backend/tracklist_selections.h"
#include "common/utils/objects.h"
#include "gui/cpp/backend/cyaml_schemas/gui/backend/tracklist_selections.h"

TracklistSelections_v2 *
tracklist_selections_upgrade_from_v1 (TracklistSelections_v1 * old)
{
  if (!old)
    return NULL;

  TracklistSelections_v2 * self = object_new (TracklistSelections_v2);

  self->schema_version = 2;
  self->num_tracks = old->num_tracks;
  for (int i = 0; i < self->num_tracks; i++)
    {
      self->tracks[i] = track_upgrade_from_v1 (old->tracks[i]);
    }
  self->is_project = old->is_project;

  return self;
}
