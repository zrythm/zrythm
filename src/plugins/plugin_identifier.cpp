// SPDX-FileCopyrightText: © 2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdio>

#include "plugins/plugin_identifier.h"
#include "utils/types.h"

#include <glib.h>

bool
PluginIdentifier::validate () const
{
  return validate_slot_type_slot_combo (slot_type_, slot_);
}

std::string
PluginIdentifier::print_to_str () const
{
  return fmt::format (
    "slot_type: %s, track_name hash: {}, slot: %d", ENUM_NAME (slot_type_),
    track_name_hash_, slot_);
}

uint32_t
PluginIdentifier::get_hash () const
{
  uint32_t hash = 0;
  hash = hash ^ g_int_hash (&slot_type_);
  hash = hash ^ track_name_hash_;
  hash = hash ^ g_int_hash (&slot_);
  return hash;
}
