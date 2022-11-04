// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/control_room.h"
#include "utils/objects.h"

#include "schemas/audio/control_room.h"

ControlRoom *
control_room_upgrade_from_v1 (ControlRoom_v1 * old)
{
  if (!old)
    return NULL;

  ControlRoom * self = object_new (ControlRoom);

  self->schema_version = CONTROL_ROOM_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  self->monitor_fader =
    fader_upgrade_from_v1 (old->monitor_fader);

  return self;
}
