// SPDX-FileCopyrightText: © 2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/plugin_identifier.h"

namespace zrythm::dsp
{

bool
PluginIdentifier::validate () const
{
  return validate_slot_type_slot_combo (slot_type_, slot_);
}

std::string
PluginIdentifier::print_to_str () const
{
  return fmt::format (
    "slot_type: {}, track_name hash: {}, slot: {}", ENUM_NAME (slot_type_),
    track_name_hash_, slot_);
}

uint32_t
PluginIdentifier::get_hash () const
{
  uint32_t hash = 0;
  hash = hash ^ qHash (slot_type_);
  hash = hash ^ track_name_hash_;
  hash = hash ^ qHash (slot_);
  return hash;
}

void
PluginIdentifier::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("slotType", slot_type_),
    make_field ("trackNameHash", track_name_hash_), make_field ("slot", slot_));
}
};
