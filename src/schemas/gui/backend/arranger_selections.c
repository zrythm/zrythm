// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/arranger_selections.h"
#include "utils/objects.h"

#include "schemas/gui/backend/arranger_selections.h"

ArrangerSelections *
arranger_selections_upgrade_from_v1 (ArrangerSelections_v1 * old)
{
  if (!old)
    return NULL;

  ArrangerSelections * self = object_new (ArrangerSelections);

  self->schema_version = ARRANGER_SELECTIONS_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  self->type = (ArrangerSelectionsType) old->type;

  return self;
}
