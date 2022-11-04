// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/region_identifier.h"
#include "utils/objects.h"

#include "schemas/audio/region_identifier.h"

RegionIdentifier *
region_identifier_upgrade_from_v1 (RegionIdentifier_v1 * old)
{
  if (!old)
    return NULL;

  RegionIdentifier * self = object_new (RegionIdentifier);

#define UPDATE(name) self->name = old->name

  self->schema_version = REGION_IDENTIFIER_SCHEMA_VERSION;

  self->type = (RegionType) old->type;
  UPDATE (link_group);
  UPDATE (track_name_hash);
  UPDATE (lane_pos);
  UPDATE (at_idx);
  UPDATE (idx);

  return self;
}
