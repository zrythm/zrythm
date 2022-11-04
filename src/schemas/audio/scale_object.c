// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/scale_object.h"
#include "utils/objects.h"
#include "schemas/audio/scale_object.h"

ScaleObject *
scale_object_upgrade_from_v1 (
  ScaleObject_v1 * old)
{
  if (!old)
    return NULL;

  ScaleObject * self = object_new (ScaleObject);

  self->schema_version = SCALE_OBJECT_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  ArrangerObject * base = arranger_object_upgrade_from_v1 (&old->base);
  self->base = *base;

  UPDATE (index);
  self->scale = musical_scale_upgrade_from_v2 (old->scale);

  return self;
}
