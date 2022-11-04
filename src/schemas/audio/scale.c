// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/scale.h"
#include "utils/objects.h"
#include "schemas/audio/scale.h"

MusicalScale *
musical_scale_upgrade_from_v2 (
  MusicalScale_v2 * old)
{
  if (!old)
    return NULL;

  MusicalScale * self = object_new (MusicalScale);

  self->schema_version = SCALE_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  self->type = (MusicalScaleType) old->type;
  self->root_key = (MusicalNote) old->root_key;

  return self;
}
