// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/settings/chord_preset.h"
#include "gui/cpp/backend/settings/chord_preset_pack.h"

#include "common/dsp/chord_descriptor.h"

void
ChordDescriptor::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("hasBass", has_bass_), make_field ("rootNote", root_note_),
    make_field ("bassNote", bass_note_), make_field ("type", type_),
    make_field ("accent", accent_), make_field ("notes", notes_),
    make_field ("inversion", inversion_));
}

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