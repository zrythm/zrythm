// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/automation_tracklist.h"
#include "utils/objects.h"

#include "schemas/audio/automation_tracklist.h"

AutomationTracklist *
automation_tracklist_upgrade_from_v1 (
  AutomationTracklist_v1 * old)
{
  if (!old)
    return NULL;

  AutomationTracklist * self =
    object_new (AutomationTracklist);

  self->schema_version = AUTOMATION_TRACKLIST_SCHEMA_VERSION;
  self->num_ats = old->num_ats;
  self->ats = g_malloc_n (
    (size_t) old->num_ats, sizeof (AutomationTrack *));
  for (int i = 0; i < self->num_ats; i++)
    {
      self->ats[i] =
        automation_track_upgrade_from_v1 (old->ats[i]);
    }

  return self;
}
