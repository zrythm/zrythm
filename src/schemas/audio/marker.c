// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/marker.h"
#include "utils/objects.h"
#include "schemas/audio/marker.h"

Marker *
marker_upgrade_from_v1 (
  Marker_v1 * old)
{
  if (!old)
    return NULL;

  Marker * self = object_new (Marker);

  self->schema_version = MARKER_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  ArrangerObject * base = arranger_object_upgrade_from_v1 (&old->base);
  self->base = *base;

  UPDATE (name);
  UPDATE (track_name_hash);
  UPDATE (index);
  self->type = (MarkerType) old->type;

  return self;
}
