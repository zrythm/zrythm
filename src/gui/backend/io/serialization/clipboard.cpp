// SPDX-FileCopyrightText: © 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/clipboard.h"

void
Clipboard::define_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<Clipboard>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_),
    T::make_field ("arrangerObjects", arranger_objects_, true),
    T::make_field ("plugins", plugins_, true),
    T::make_field ("tracklistSelections", tracks_, true));
}
