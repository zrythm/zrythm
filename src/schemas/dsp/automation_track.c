// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/automation_track.h"
#include "utils/objects.h"

#include "schemas/dsp/automation_track.h"

AutomationTrack *
automation_track_upgrade_from_v1 (AutomationTrack_v1 * old)
{
  if (!old)
    return NULL;

  AutomationTrack * self = object_new (AutomationTrack);

  self->schema_version = AUTOMATION_TRACK_SCHEMA_VERSION;
  self->index = old->index;
  PortIdentifier * port_id = port_identifier_upgrade_from_v1 (&old->port_id);
  self->port_id = *port_id;
  self->created = old->created;
  self->num_regions = old->num_regions;
  self->regions = g_malloc_n ((size_t) old->num_regions, sizeof (ZRegion *));
  for (int i = 0; i < self->num_regions; i++)
    {
      self->regions[i] = region_upgrade_from_v1 (old->regions[i]);
    }
  self->visible = old->visible;
  self->height = old->height;
  self->automation_mode = (AutomationMode) old->automation_mode;

  return self;
}
