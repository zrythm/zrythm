// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/sample_processor.h"
#include "utils/objects.h"
#include "schemas/audio/sample_processor.h"

SampleProcessor *
sample_processor_upgrade_from_v1 (
  SampleProcessor_v1 * old)
{
  if (!old)
    return NULL;

  SampleProcessor * self = object_new (SampleProcessor);

  self->schema_version = SAMPLE_PROCESSOR_SCHEMA_VERSION;
  self->fader = fader_upgrade_from_v1 (old->fader);

  return self;
}
