// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/curve.h"
#include "utils/objects.h"

#include "schemas/audio/curve.h"

CurveOptions *
curve_options_upgrade_from_v1 (CurveOptions_v1 * old)
{
  if (!old)
    return NULL;

  CurveOptions * self = object_new (CurveOptions);

  self->schema_version = CURVE_OPTIONS_SCHEMA_VERSION;
  self->algo = (CurveAlgorithm) old->algo;
  self->curviness = old->curviness;

  return self;
}
