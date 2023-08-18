// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/velocity.h"
#include "utils/objects.h"

#include "schemas/dsp/velocity.h"

Velocity *
velocity_upgrade_from_v1 (Velocity_v1 * old)
{
  if (!old)
    return NULL;

  Velocity * self = object_new (Velocity);

  self->schema_version = VELOCITY_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  ArrangerObject * base =
    arranger_object_upgrade_from_v1 (&old->base);
  self->base = *base;

  UPDATE (vel);

  return self;
}
