// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/uuid_identifiable_object.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm
{

class BaseTestObject : public utils::UuidIdentifiableObject<BaseTestObject>
{
  Q_OBJECT
public:
  BaseTestObject (QObject * parent = nullptr)
      : utils::UuidIdentifiableObject<BaseTestObject> (parent)
  {
  }
  explicit BaseTestObject (Uuid id, QObject * parent = nullptr)
      : utils::UuidIdentifiableObject<BaseTestObject> (id, parent)
  {
  }
  ~BaseTestObject () override = default;
  BaseTestObject (const BaseTestObject &) = delete;
  BaseTestObject &operator= (const BaseTestObject &) = delete;
  BaseTestObject (BaseTestObject &&) = delete;
  BaseTestObject &operator= (BaseTestObject &&) = delete;

  friend void to_json (nlohmann::json &json_value, const BaseTestObject &obj)
  {
    to_json (json_value, static_cast<const UuidIdentifiableObject &> (obj));
  }
  friend void from_json (const nlohmann::json &json_value, BaseTestObject &obj)
  {
    from_json (json_value, static_cast<UuidIdentifiableObject &> (obj));
  }

  BOOST_DESCRIBE_CLASS (BaseTestObject, (UuidIdentifiableObject), (), (), ())
};

static_assert (utils::UuidIdentifiable<BaseTestObject>);

}

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::BaseTestObject::Uuid);

namespace zrythm
{

using TestUuid = BaseTestObject::Uuid;

class DerivedTestObject : public BaseTestObject
{
  Q_OBJECT
public:
  // Used to test non-default constructable objects that require a dependency
  // during deserialization
  explicit DerivedTestObject (int some_dependency) { };
  explicit DerivedTestObject (TestUuid id, std::string name)
      : BaseTestObject (id), name_ (std::move (name))
  {
  }

  [[nodiscard]] std::string name () const { return name_; }

  friend void init_from (
    DerivedTestObject       &obj,
    const DerivedTestObject &other,
    utils::ObjectCloneType   clone_type)
  {
    obj.name_ = other.name_;
  }

  NLOHMANN_DEFINE_DERIVED_TYPE_INTRUSIVE (DerivedTestObject, BaseTestObject, name_)

private:
  std::string name_;

  BOOST_DESCRIBE_CLASS (DerivedTestObject, (BaseTestObject), (), (), (name_))
};

}
