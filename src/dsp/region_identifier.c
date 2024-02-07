/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "dsp/region_identifier.h"
#include "utils/objects.h"

void
region_identifier_init (RegionIdentifier * self)
{
}

bool
region_identifier_validate (RegionIdentifier * self)
{
  /* TODO */
  return true;
}

void
region_identifier_free (RegionIdentifier * self)
{
  object_zero_and_free (self);
}
