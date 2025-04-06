// SPDX-FileCopyrightText: © 2021, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/plugin_slot.h"

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

};