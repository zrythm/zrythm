// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"
#include "utils/objects.h"

#include "schemas/audio/position.h"

Position *
position_upgrade_from_v1 (Position_v1 * old)
{
  if (!old)
    return NULL;

  Position * self = object_new (Position);

  self->schema_version = POSITION_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  UPDATE (ticks);
  UPDATE (frames);

  return self;
}
