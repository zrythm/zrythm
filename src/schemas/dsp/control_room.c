// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/control_room.h"
#include "schemas/dsp/control_room.h"
#include "utils/objects.h"

ControlRoom_v2 *
control_room_upgrade_from_v1 (ControlRoom_v1 * old)
{
  if (!old)
    return NULL;

  ControlRoom_v2 * self = object_new (ControlRoom_v2);

  self->schema_version = 2;
  self->monitor_fader = fader_upgrade_from_v1 (old->monitor_fader);

  return self;
}
