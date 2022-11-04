// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/chord_object.h"
#include "utils/objects.h"

#include "schemas/audio/chord_object.h"

ChordObject *
chord_object_upgrade_from_v1 (ChordObject_v1 * old)
{
  if (!old)
    return NULL;

  ChordObject * self = object_new (ChordObject);

  self->schema_version = CHORD_OBJECT_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  ArrangerObject * base =
    arranger_object_upgrade_from_v1 (&old->base);
  self->base = *base;

  UPDATE (index);
  UPDATE (chord_index);

  return self;
}
