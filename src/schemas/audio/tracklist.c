// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/tracklist.h"
#include "utils/objects.h"
#include "schemas/audio/tracklist.h"

Tracklist *
tracklist_upgrade_from_v1 (
  Tracklist_v1 * old)
{
  if (!old)
    return NULL;

  Tracklist * self = object_new (Tracklist);

  self->schema_version = TRACKLIST_SCHEMA_VERSION;
  self->num_tracks = old->num_tracks;
  for (int i = 0; i < self->num_tracks; i++)
    {
      self->tracks[i] = track_upgrade_from_v1 (old->tracks[i]);
    }
  self->pinned_tracks_cutoff = old->pinned_tracks_cutoff;

  return self;
}
