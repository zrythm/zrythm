// SPDX-FileCopyrightText: © 2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdio.h>

#include "plugins/plugin_identifier.h"
#include "utils/debug.h"

#include <glib.h>

void
plugin_identifier_init (PluginIdentifier * self)
{
  memset (self, 0, sizeof (PluginIdentifier));
  self->slot = -1;
}

bool
plugin_identifier_validate (const PluginIdentifier * self)
{
  /*z_return_val_if_fail_cmp (*/
  g_return_val_if_fail (
    plugin_identifier_validate_slot_type_slot_combo (self->slot_type, self->slot),
    false);
  return true;
}

/**
 * Verifies that @ref slot_type and @ref slot is
 * a valid combination.
 */
bool
plugin_identifier_validate_slot_type_slot_combo (
  ZPluginSlotType slot_type,
  int             slot)
{
  return (slot_type == Z_PLUGIN_SLOT_INSTRUMENT && slot == -1)
         || (slot_type == Z_PLUGIN_SLOT_INVALID && slot == -1)
         || (slot_type != Z_PLUGIN_SLOT_INSTRUMENT && slot >= 0);
}

void
plugin_identifier_copy (PluginIdentifier * dest, const PluginIdentifier * src)
{
  g_return_if_fail (plugin_identifier_validate (src));

  dest->slot_type = src->slot_type;
  dest->track_name_hash = src->track_name_hash;
  dest->slot = src->slot;
}

void
plugin_identifier_print (const PluginIdentifier * self, char * str)
{
  sprintf (
    str, "slot_type: %d, track_name hash: %u, slot: %d", self->slot_type,
    self->track_name_hash, self->slot);
}

uint32_t
plugin_identifier_get_hash (const void * id)
{
  const PluginIdentifier * self = (const PluginIdentifier *) id;
  uint32_t                 hash = 0;
  hash = hash ^ g_int_hash (&self->slot_type);
  hash = hash ^ self->track_name_hash;
  hash = hash ^ g_int_hash (&self->slot);
  return hash;
}
