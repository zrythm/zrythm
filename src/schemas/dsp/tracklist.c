// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "schemas/dsp/tracklist.h"
#include "utils/objects.h"

Tracklist_v2 *
tracklist_upgrade_from_v1 (Tracklist_v1 * old)
{
  if (!old)
    return NULL;

  Tracklist_v2 * self = object_new (Tracklist_v2);

  self->schema_version = 2;
  self->num_tracks = old->num_tracks;
  for (int i = 0; i < self->num_tracks; i++)
    {
      self->tracks[i] = track_upgrade_from_v1 (old->tracks[i]);
    }
  self->pinned_tracks_cutoff = old->pinned_tracks_cutoff;

  return self;
}
