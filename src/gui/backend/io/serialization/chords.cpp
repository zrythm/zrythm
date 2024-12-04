// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "gui/backend/backend/settings/chord_preset.h"
#include "gui/backend/backend/settings/chord_preset_pack.h"

void
ChordPreset::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("name", name_), make_field ("descriptors", descr_));
}

void
ChordPresetPack::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("name", name_), make_field ("presets", presets_),
    make_field ("isStandard", is_standard_));
}
