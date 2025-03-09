// SPDX-FileCopyrightText: © 2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/plugin_identifier.h"

namespace zrythm::dsp
{

void
PluginSlot::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("type", type_), make_field ("index", slot_));
}

uint32_t
PluginSlot::get_hash () const
{
  uint32_t hash = 0;
  if (slot_.has_value ())
    hash = hash ^ qHash (slot_.value ());
  hash = hash ^ qHash (type_);
  return hash;
}

bool
PluginIdentifier::validate () const
{
  return slot_.validate_slot_type_slot_combo ();
}

std::string
PluginIdentifier::print_to_str () const
{
  return fmt::format (
    "track_name hash: {}, slot: {}", type_safe::get (track_id_), slot_);
}

uint32_t
PluginIdentifier::get_hash () const
{
  uint32_t hash = 0;
  hash = hash ^ slot_.get_hash ();
  hash = hash ^ qHash (type_safe::get (track_id_));
  return hash;
}

void
PluginIdentifier::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("trackId", track_id_), make_field ("slot", slot_));
}
};
