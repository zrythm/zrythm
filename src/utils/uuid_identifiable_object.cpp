// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/uuid_identifiable_object.h"

namespace zrythm::utils
{

void
UuidIdentifiableObject::define_base_fields (
  const serialization::ISerializableBase::Context &ctx)
{
  serialize_fields (ctx, make_field ("id", id_));
}

} // namespace zrythm::utils
