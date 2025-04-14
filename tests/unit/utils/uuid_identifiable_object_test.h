// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/gtest_wrapper.h"
#include "utils/uuid_identifiable_object.h"

using namespace zrythm::utils;

using namespace zrythm;

class BaseTestObject
    : public utils::UuidIdentifiableObject<BaseTestObject>,
      public serialization::ISerializable<BaseTestObject>
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

  std::string get_document_type () const override { return "TestObject"; }
  void        define_fields (const ISerializableBase::Context &ctx)
  {
    UuidIdentifiableObject::define_base_fields (ctx);
  }
};

static_assert (UuidIdentifiable<BaseTestObject>);
static_assert (!UuidIdentifiableQObject<BaseTestObject>);

DEFINE_UUID_HASH_SPECIALIZATION (BaseTestObject::Uuid);

using TestUuid = BaseTestObject::Uuid;

class DerivedTestObject
    : public QObject,
      public BaseTestObject,
      public ICloneable<DerivedTestObject>
{
  Q_OBJECT
public:
  explicit DerivedTestObject (TestUuid id, std::string name)
      : BaseTestObject (id), name_ (std::move (name))
  {
  }

  [[nodiscard]] std::string name () const { return name_; }

  void
  init_after_cloning (const DerivedTestObject &other, ObjectCloneType clone_type)
    final
  {
    name_ = other.name_;
  }

  Q_SIGNAL void selectedChanged (bool selected);

private:
  std::string name_;
};

// FIXME!!!
// static_assert (UuidIdentifiableQObject<DerivedTestObject>);

using TestVariant = std::variant<DerivedTestObject *>;
using TestRegistry = utils::OwningObjectRegistry<TestVariant, BaseTestObject>;

class UuidIdentifiableObjectRegistryTest : public ::testing::Test
{
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

class UuidIdentifiableObjectSelectionManagerTest
    : public QObject,
      public ::testing::Test
{
  Q_OBJECT
protected:
  void SetUp () override
  {
    obj1_ = new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "Object1");
    obj2_ = new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "Object2");
    obj3_ = new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "Object3");

    registry_.register_object (obj1_);
    registry_.register_object (obj2_);
    registry_.register_object (obj3_);
    selection_manager_ = std::make_unique<
      utils::UuidIdentifiableObjectSelectionManager<TestRegistry>> (
      selected_objects_, registry_);
  }

  TestRegistry        registry_;
  DerivedTestObject * obj1_{};
  DerivedTestObject * obj2_{};
  DerivedTestObject * obj3_{};

  UuidIdentifiableObjectSelectionManager<TestRegistry>::UuidSet selected_objects_;
  std::unique_ptr<utils::UuidIdentifiableObjectSelectionManager<TestRegistry>>
    selection_manager_;
};

static_assert (
  std::copy_constructible<UuidIdentifiableObjectSelectionManager<TestRegistry>>);
static_assert (
  std::move_constructible<UuidIdentifiableObjectSelectionManager<TestRegistry>>);