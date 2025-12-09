// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/uuid_identifiable_object.h"

#include <gtest/gtest.h>

using namespace zrythm::utils;

using namespace zrythm;

class BaseTestObject : public utils::UuidIdentifiableObject<BaseTestObject>
{
public:
  BaseTestObject () = default;
  explicit BaseTestObject (Uuid id)
      : UuidIdentifiableObject<BaseTestObject> (id)
  {
  }
  ~BaseTestObject () override = default;
  BaseTestObject (const BaseTestObject &) = default;
  BaseTestObject &operator= (const BaseTestObject &) = default;
  BaseTestObject (BaseTestObject &&) = default;
  BaseTestObject &operator= (BaseTestObject &&) = default;

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

static_assert (UuidIdentifiable<BaseTestObject>);
static_assert (!UuidIdentifiableQObject<BaseTestObject>);

DEFINE_UUID_HASH_SPECIALIZATION (BaseTestObject::Uuid);

using TestUuid = BaseTestObject::Uuid;

class DerivedTestObject : public QObject, public BaseTestObject
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
    ObjectCloneType          clone_type)

  {
    obj.name_ = other.name_;
  }

  NLOHMANN_DEFINE_DERIVED_TYPE_INTRUSIVE (DerivedTestObject, BaseTestObject, name_)

private:
  std::string name_;

  BOOST_DESCRIBE_CLASS (DerivedTestObject, (BaseTestObject), (), (), (name_))
};

class TestObjectBuilder
{
public:
  TestObjectBuilder () = default;
  TestObjectBuilder &with_int_dependency (int some_dependency)
  {
    some_dependency_ = some_dependency;
    return *this;
  }

  template <typename ObjectT> auto build () const
  {
    return std::make_unique<ObjectT> (some_dependency_);
  }

private:
  int some_dependency_{};
};
static_assert (ObjectBuilder<TestObjectBuilder>);

// FIXME!!!
// static_assert (UuidIdentifiableQObject<DerivedTestObject>);

class UuidIdentifiableObjectRegistryTest : public ::testing::Test
{
public:
  using TestVariant = std::variant<DerivedTestObject *>;
  using TestRegistry = utils::OwningObjectRegistry<TestVariant, BaseTestObject>;

protected:
  void SetUp () override
  {
    obj1_ = new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "Object1");
    obj2_ = new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "Object2");
    obj3_ = new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "Object3");

    registry_.register_object (obj1_);
    registry_.register_object (obj2_);
    registry_.register_object (obj3_);
  }

  TestRegistry        registry_;
  DerivedTestObject * obj1_{};
  DerivedTestObject * obj2_{};
  DerivedTestObject * obj3_{};
};
