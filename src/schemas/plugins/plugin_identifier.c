// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_identifier.h"
#include "utils/objects.h"
#include "schemas/plugins/plugin_identifier.h"

PluginIdentifier *
plugin_identifier_upgrade_from_v1 (
  PluginIdentifier_v1 * old)
{
  if (!old)
    return NULL;

  PluginIdentifier * self = object_new (PluginIdentifier);
  self->schema_version = PLUGIN_IDENTIFIER_SCHEMA_VERSION;
  self->slot_type = (PluginSlotType) old->slot_type;
  self->track_name_hash = old->track_name_hash;
  self->slot = old->slot;

  return self;
}
