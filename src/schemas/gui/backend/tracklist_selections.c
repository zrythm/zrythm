// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/tracklist_selections.h"
#include "utils/objects.h"
#include "schemas/gui/backend/tracklist_selections.h"

TracklistSelections *
tracklist_selections_upgrade_from_v1 (
  TracklistSelections_v1 * old)
{
  if (!old)
    return NULL;

  TracklistSelections * self = object_new (TracklistSelections);

  self->schema_version = TRACKLIST_SELECTIONS_SCHEMA_VERSION;
  self->num_tracks = old->num_tracks;
  for (int i = 0; i < self->num_tracks; i++)
    {
      self->tracks[i] = track_upgrade_from_v1 (old->tracks[i]);
    }
  self->is_project = old->is_project;

  return self;
}
