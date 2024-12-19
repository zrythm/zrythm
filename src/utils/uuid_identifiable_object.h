// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/iserializable.h"

#include <QUuid>

namespace zrythm::utils
{

/**
 * Base class for objects that need to be uniquely identified by UUID.
 */
class UuidIdentifiableObject
    : virtual public serialization::ISerializable<UuidIdentifiableObject>
{
public:
  using Id = QUuid;

  UuidIdentifiableObject () : id_ (QUuid::createUuid ()) { }
  UuidIdentifiableObject (const Id &id) : id_ (id) { }
  UuidIdentifiableObject (const UuidIdentifiableObject &other) = default;
  UuidIdentifiableObject &
  operator= (const UuidIdentifiableObject &other) = default;
  UuidIdentifiableObject (UuidIdentifiableObject &&other) = default;
  UuidIdentifiableObject &operator= (UuidIdentifiableObject &&other) = default;
  ~UuidIdentifiableObject () override = default;

  Id get_uuid () const { return id_; }

protected:
  DECLARE_DEFINE_BASE_FIELDS_METHOD ()

  Id id_;
};

} // namespace zrythm::utils
