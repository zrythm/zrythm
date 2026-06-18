// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/object_registry.h"
#include "utils/registry_utils.h"
#include "utils/typed_uuid_reference.h"
#include "utils/uuid_identifiable_object.h"
#include "utils/uuid_reference.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::utils
{

/**
 * @brief Simple QObject test object with a name, for testing UuidReference
 * and TypedUuidReference.
 */
class TestObject : public utils::UuidIdentifiableObject<TestObject>
{
  Q_OBJECT
public:
  TestObject (QObject * parent = nullptr)
      : utils::UuidIdentifiableObject<TestObject> (parent)
  {
  }
  TestObject (Uuid id, std::string name, QObject * parent = nullptr)
      : utils::UuidIdentifiableObject<TestObject> (id, parent),
        name_ (std::move (name))
  {
  }
  ~TestObject () override = default;

  [[nodiscard]] std::string name () const { return name_; }

  friend void
  init_from (TestObject &obj, const TestObject &other, ObjectCloneType)
  {
    obj.name_ = other.name_;
  }

  NLOHMANN_DEFINE_DERIVED_TYPE_INTRUSIVE (
    TestObject,
    UuidIdentifiableObject,
    name_)

private:
  std::string name_;

  BOOST_DESCRIBE_CLASS (TestObject, (UuidIdentifiableObject), (), (), (name_))
};

static_assert (UuidIdentifiable<TestObject>);

class DerivedUuidRefTestObject : public TestObject
{
  Q_OBJECT
public:
  using TestObject::TestObject;
};
static_assert (UuidIdentifiable<DerivedUuidRefTestObject>);

/**
 * @brief Mock IObjectRegistry for testing reference behavior in isolation.
 *
 * Use this when testing that UuidReference/TypedUuidReference correctly call
 * acquire/release on the registry, without needing a real implementation.
 */
class MockObjectRegistry : public utils::IObjectRegistry
{
public:
  MOCK_METHOD (
    void,
    register_object_impl,
    (utils::UuidIdentifiableBase &),
    (override));
  MOCK_METHOD (void, acquire_reference_impl, (const QUuid &), (override));
  MOCK_METHOD (void, release_reference_impl, (const QUuid &), (override));
  MOCK_METHOD (
    utils::UuidIdentifiableBase *,
    find_by_raw_uuid_impl,
    (const QUuid &),
    (const, override));
  MOCK_METHOD (bool, contains_impl, (const QUuid &), (const, override));

protected:
  MOCK_METHOD (
    void,
    for_each_matching_impl,
    (const QMetaObject &, ObjectVisitor),
    (const, override));
};

/**
 * @brief Test object that holds a TypedUuidReference to another object.
 *
 * Used to test proper destruction order when the registry is destroyed
 * while objects contain references to other objects in the same registry.
 */
class ContainerTestObject
    : public utils::UuidIdentifiableObject<ContainerTestObject>
{
  Q_OBJECT
public:
  ContainerTestObject (QObject * parent = nullptr)
      : utils::UuidIdentifiableObject<ContainerTestObject> (parent)
  {
  }
  ContainerTestObject (Uuid id, std::string name, QObject * parent = nullptr)
      : utils::UuidIdentifiableObject<ContainerTestObject> (id, parent),
        name_ (std::move (name))
  {
  }

  using UuidRef = TypedUuidReference<ContainerTestObject>;
  void set_contained_ref (UuidRef ref) { contained_ref_ = std::move (ref); }
  [[nodiscard]] std::string name () const { return name_; }

  friend void init_from (
    ContainerTestObject       &obj,
    const ContainerTestObject &other,
    ObjectCloneType)
  {
    obj.name_ = other.name_;
  }

  NLOHMANN_DEFINE_DERIVED_TYPE_INTRUSIVE (
    ContainerTestObject,
    utils::UuidIdentifiableObject<ContainerTestObject>,
    name_)

private:
  std::string                                            name_;
  std::optional<TypedUuidReference<ContainerTestObject>> contained_ref_;

  BOOST_DESCRIBE_CLASS (
    ContainerTestObject,
    (utils::UuidIdentifiableObject<ContainerTestObject>),
    (),
    (),
    (name_))
};

static_assert (UuidIdentifiable<ContainerTestObject>);
}

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::utils::TestObject::Uuid)

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::utils::ContainerTestObject::Uuid)
