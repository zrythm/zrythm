// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/automation_point.h"
#include "utils/objects.h"
#include "schemas/audio/automation_point.h"

AutomationPoint *
automation_point_upgrade_from_v1 (
  AutomationPoint_v1 * old)
{
  if (!old)
    return NULL;

  AutomationPoint * self = object_new (AutomationPoint);

  self->schema_version = AUTOMATION_POINT_SCHEMA_VERSION;
  ArrangerObject * base = arranger_object_upgrade_from_v1 (&old->base);
  self->base = *base;
  self->fvalue = old->fvalue;
  self->normalized_val = old->normalized_val;
  self->index = old->index;
  CurveOptions * curve_opts = curve_options_upgrade_from_v1 (&old->curve_opts);
  self->curve_opts = *curve_opts;

  return self;
}
