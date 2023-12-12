// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/sample_processor.h"
#include "schemas/dsp/sample_processor.h"
#include "utils/objects.h"

SampleProcessor_v2 *
sample_processor_upgrade_from_v1 (SampleProcessor_v1 * old)
{
  if (!old)
    return NULL;

  SampleProcessor_v2 * self = object_new (SampleProcessor_v2);

  self->schema_version = 2;
  self->fader = fader_upgrade_from_v1 (old->fader);

  return self;
}
