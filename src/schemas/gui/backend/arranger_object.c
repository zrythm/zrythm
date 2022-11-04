// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/arranger_object.h"
#include "utils/objects.h"
#include "schemas/gui/backend/arranger_object.h"

ArrangerObject *
arranger_object_upgrade_from_v1 (
  ArrangerObject_v1 * old)
{
  if (!old)
    return NULL;

  ArrangerObject * self = object_new (ArrangerObject);

#define UPDATE_POS(name) \
  { \
    Position * name = position_upgrade_from_v1 (&old->name); \
    self->name = *name; \
  }

  self->schema_version = ARRANGER_OBJECT_SCHEMA_VERSION;
  self->type = (ArrangerObjectType) old->type;
  self->flags = (ArrangerObjectFlags) old->flags;
  self->muted = old->muted;
  UPDATE_POS (pos);
  UPDATE_POS (end_pos);
  UPDATE_POS (clip_start_pos);
  UPDATE_POS (loop_start_pos);
  UPDATE_POS (loop_end_pos);
  UPDATE_POS (fade_in_pos);
  UPDATE_POS (fade_out_pos);
  CurveOptions * fade_in_opts = curve_options_upgrade_from_v1 (&old->fade_in_opts);
  self->fade_in_opts = *fade_in_opts;
  CurveOptions * fade_out_opts = curve_options_upgrade_from_v1 (&old->fade_out_opts);
  self->fade_out_opts = *fade_out_opts;
  RegionIdentifier * region_id = region_identifier_upgrade_from_v1 (&old->region_id);
  self->region_id = *region_id;

  return self;
}
