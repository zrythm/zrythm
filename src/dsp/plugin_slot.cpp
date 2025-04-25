// SPDX-FileCopyrightText: © 2021, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/plugin_slot.h"

namespace zrythm::dsp
{

void
PluginSlot::define_fields (const utils::serialization::Context &ctx)
{
  serialize_fields (
    ctx, make_field ("type", type_), make_field ("index", slot_));
}

size_t
PluginSlot::get_hash () const
{
  size_t hash{};
  if (slot_.has_value ())
    hash = hash ^ qHash (slot_.value ());
  hash = hash ^ qHash (type_);
  return hash;
}

};
