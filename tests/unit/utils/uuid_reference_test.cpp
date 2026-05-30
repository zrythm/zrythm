// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/serialization.h"

#include "./uuid_reference_test.h"
#include <gmock/gmock.h>

namespace zrythm::utils
{

using TestUuid = TestObject::Uuid;

// ============================================================================
// IObjectRegistry tests
// ============================================================================

TEST (IObjectRegistryTest, FindByRawUuidOrThrowReturnsObject)
{
  utils::ObjectRegistry registry;
  auto *     obj = new TestObject (TestUuid{ QUuid::createUuid () }, "found");
  const auto id = obj->raw_uuid ();
  registry.register_object (*obj);

  auto * result = registry.find_by_raw_uuid_or_throw (id);
  EXPECT_EQ (result, obj);
}

TEST (IObjectRegistryTest, FindByRawUuidOrThrowThrowsOnMissing)
{
  utils::ObjectRegistry registry;
  EXPECT_THROW (
    registry.find_by_raw_uuid_or_throw (QUuid::createUuid ()),
    std::runtime_error);
}

// ============================================================================
// UuidReference tests (using mock for behavioral verification)
// ============================================================================

class UuidReferenceTest : public ::testing::Test
{
protected:
  MockObjectRegistry mock_registry_;
  TestObject         obj_{ TestUuid{ QUuid::createUuid () }, "test" };
  QUuid              id_ = obj_.raw_uuid ();
};

TEST_F (UuidReferenceTest, RegistryOnlyConstructionIsUnengaged)
{
  UuidReference ref (mock_registry_);
  EXPECT_FALSE (ref.has_value ());
  EXPECT_EQ (ref.get (), nullptr);
}

TEST_F (UuidReferenceTest, ConstructionAcquiresReference)
{
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id_)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (id_)).Times (1);

  UuidReference ref (id_, mock_registry_);
  EXPECT_TRUE (ref.has_value ());
  EXPECT_EQ (ref.id (), id_);
}

TEST_F (UuidReferenceTest, DestructionReleasesReference)
{
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id_)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (id_)).Times (1);

  {
    UuidReference ref (id_, mock_registry_);
  }
}

TEST_F (UuidReferenceTest, CopyConstructionAcquiresNewReference)
{
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id_)).Times (2);
  EXPECT_CALL (mock_registry_, release_reference_impl (id_)).Times (2);

  UuidReference ref1 (id_, mock_registry_);
  UuidReference ref2 (ref1);
  EXPECT_EQ (ref2.id (), id_);
}

TEST_F (UuidReferenceTest, MoveConstructionTransfersOwnership)
{
  // Move: acquire once (ref1), release once (moved-from ref2 destructor is
  // no-op since id is nullopt)
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id_)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (id_)).Times (1);

  UuidReference ref1 (id_, mock_registry_);
  UuidReference ref2 (std::move (ref1));
  EXPECT_FALSE (ref1.has_value ());
  EXPECT_TRUE (ref2.has_value ());
  EXPECT_EQ (ref2.id (), id_);
}

TEST_F (UuidReferenceTest, CopyAssignmentAcquiresAndReleases)
{
  QUuid id2 = QUuid::createUuid ();
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id_)).Times (2);
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id2)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (id_)).Times (2);
  EXPECT_CALL (mock_registry_, release_reference_impl (id2)).Times (1);

  UuidReference ref1 (id_, mock_registry_);
  UuidReference ref2 (id2, mock_registry_);
  ref2 = ref1;
  EXPECT_EQ (ref2.id (), id_);
}

TEST_F (UuidReferenceTest, MoveAssignmentTransfersOwnership)
{
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id_)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (id_)).Times (1);

  UuidReference ref1 (id_, mock_registry_);
  UuidReference ref2 (mock_registry_);
  ref2 = std::move (ref1);
  EXPECT_FALSE (ref1.has_value ());
  EXPECT_TRUE (ref2.has_value ());
}

TEST_F (UuidReferenceTest, GetReturnsObject)
{
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id_)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (id_)).Times (1);
  EXPECT_CALL (mock_registry_, find_by_raw_uuid_impl (id_))
    .WillOnce (testing::Return (&obj_));

  UuidReference ref (id_, mock_registry_);
  auto *        result = ref.get ();
  EXPECT_EQ (result, &obj_);
}

TEST_F (UuidReferenceTest, GetReturnsNullWhenUnengaged)
{
  UuidReference ref (mock_registry_);
  EXPECT_EQ (ref.get (), nullptr);
}

TEST_F (UuidReferenceTest, GetOrThrowThrowsWhenUnengaged)
{
  UuidReference ref (mock_registry_);
  EXPECT_THROW (ref.get_or_throw (), std::runtime_error);
}

TEST_F (UuidReferenceTest, GetOrThrowReturnsObject)
{
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id_)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (id_)).Times (1);
  EXPECT_CALL (mock_registry_, find_by_raw_uuid_impl (id_))
    .WillOnce (testing::Return (&obj_));

  UuidReference ref (id_, mock_registry_);
  EXPECT_EQ (ref.get_or_throw (), &obj_);
}

TEST_F (UuidReferenceTest, SetIdAcquiresReference)
{
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id_)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (id_)).Times (1);

  UuidReference ref (mock_registry_);
  EXPECT_FALSE (ref.has_value ());
  ref.set_id (id_);
  EXPECT_TRUE (ref.has_value ());
}

TEST_F (UuidReferenceTest, SetIdThrowsIfAlreadySet)
{
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id_)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (id_)).Times (1);

  UuidReference ref (id_, mock_registry_);
  EXPECT_THROW (ref.set_id (id_), std::runtime_error);
}

TEST_F (UuidReferenceTest, EqualityBasedOnUuid)
{
  EXPECT_CALL (mock_registry_, acquire_reference_impl (testing::_)).Times (2);
  EXPECT_CALL (mock_registry_, release_reference_impl (testing::_)).Times (2);

  UuidReference ref1 (id_, mock_registry_);
  UuidReference ref2 (id_, mock_registry_);
  EXPECT_EQ (ref1, ref2);

  QUuid other_id = QUuid::createUuid ();
  EXPECT_CALL (mock_registry_, acquire_reference_impl (other_id)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (other_id)).Times (1);
  UuidReference ref3 (other_id, mock_registry_);
  EXPECT_NE (ref1, ref3);
}

TEST_F (UuidReferenceTest, SerializationRoundTrip)
{
  EXPECT_CALL (mock_registry_, acquire_reference_impl (id_)).Times (2);
  EXPECT_CALL (mock_registry_, release_reference_impl (id_)).Times (2);

  UuidReference  ref1 (id_, mock_registry_);
  nlohmann::json j;
  to_json (j, ref1);

  UuidReference ref2 (mock_registry_);
  from_json (j, ref2);
  EXPECT_TRUE (ref2.has_value ());
  EXPECT_EQ (ref2.id (), id_);
}

TEST_F (UuidReferenceTest, DeserializationReplacesExistingId)
{
  const QUuid old_id = id_;
  const QUuid new_id = QUuid::createUuid ();

  EXPECT_CALL (mock_registry_, acquire_reference_impl (old_id)).Times (1);
  EXPECT_CALL (mock_registry_, acquire_reference_impl (new_id)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (old_id)).Times (1);
  EXPECT_CALL (mock_registry_, release_reference_impl (new_id)).Times (1);

  UuidReference ref (old_id, mock_registry_);
  EXPECT_TRUE (ref.has_value ());
  EXPECT_EQ (ref.id (), old_id);

  nlohmann::json j = new_id;
  from_json (j, ref);
  EXPECT_TRUE (ref.has_value ());
  EXPECT_EQ (ref.id (), new_id);
}

TEST_F (UuidReferenceTest, SerializationNullWhenUnengaged)
{
  UuidReference  ref (mock_registry_);
  nlohmann::json j;
  to_json (j, ref);
  EXPECT_TRUE (j.is_null ());
}

// ============================================================================
// UuidReference lifecycle tests (using real utils::ObjectRegistry)
// ============================================================================

TEST (UuidReferenceLifecycleTest, ReferenceCountingWithCopies)
{
  utils::ObjectRegistry registry;
  auto                  ref = std::make_optional (
    utils::create_object<TestObject> (
      registry, TestUuid{ QUuid::createUuid () }, "RefCounted"));
  const auto id = type_safe::get (ref->id ());

  EXPECT_TRUE (registry.contains (id));
  EXPECT_EQ (registry.count_matching<TestObject> (), 1);
  EXPECT_EQ (registry.ref_count (id), 1);

  {
    auto ref2 = *ref;
    EXPECT_TRUE (registry.contains (id));
    EXPECT_EQ (registry.ref_count (id), 2);
  }

  EXPECT_TRUE (registry.contains (id));
  EXPECT_EQ (registry.ref_count (id), 1);

  ref.reset ();
  EXPECT_FALSE (registry.contains (id));
  EXPECT_EQ (registry.count_matching<TestObject> (), 0);
}

TEST (UuidReferenceLifecycleTest, ReferenceCountingWithMoves)
{
  utils::ObjectRegistry registry;
  auto                  ref = utils::create_object<TestObject> (
    registry, TestUuid{ QUuid::createUuid () }, "MoveTest");
  auto id = type_safe::get (ref.id ());

  EXPECT_EQ (registry.ref_count (id), 1);

  {
    auto moved_ref (std::move (ref));
    EXPECT_TRUE (registry.contains (id));
    EXPECT_EQ (registry.ref_count (id), 1);
  }

  EXPECT_FALSE (registry.contains (id));
}

TEST (UuidReferenceLifecycleTest, ReferenceCountingWithMoveAssignment)
{
  utils::ObjectRegistry registry;
  auto                  ref = utils::create_object<TestObject> (
    registry, TestUuid{ QUuid::createUuid () }, "MoveAssign");
  auto id = type_safe::get (ref.id ());

  EXPECT_EQ (registry.ref_count (id), 1);

  {
    auto moved_ref = std::move (ref);
    EXPECT_TRUE (registry.contains (id));
    EXPECT_EQ (registry.ref_count (id), 1);
  }

  EXPECT_FALSE (registry.contains (id));
}

TEST (UuidReferenceLifecycleTest, ReserializationPreservesObjectWhenUuidUnchanged)
{
  utils::ObjectRegistry registry;
  auto                  uuid = TestUuid{ QUuid::createUuid () };
  auto ref = utils::create_object<TestObject> (registry, uuid, "persist");
  auto raw_id = type_safe::get (uuid);

  EXPECT_EQ (registry.ref_count (raw_id), 1);

  nlohmann::json j;
  to_json (j, ref);

  from_json (j, ref);

  EXPECT_TRUE (registry.contains (raw_id));
  EXPECT_EQ (registry.ref_count (raw_id), 1);
  EXPECT_NE (ref.get (), nullptr);
  EXPECT_EQ (ref.get ()->name (), "persist");
}

TEST (UuidReferenceLifecycleTest, RegistryDestructionSkipsRefcounting)
{
  auto * registry = new utils::ObjectRegistry;
  auto * obj =
    new TestObject (TestUuid{ QUuid::createUuid () }, "DestructionTest");
  registry->register_object (*obj);

  // Deleting the registry should not crash (destroying_ flag prevents
  // refcount operations during Qt child destruction)
  delete registry;
}

// ============================================================================
// TypedUuidReference tests
// ============================================================================

class TypedUuidReferenceTest : public ::testing::Test
{
protected:
  utils::ObjectRegistry registry_;
};

TEST_F (TypedUuidReferenceTest, RegistryOnlyConstructionIsUnengaged)
{
  TypedUuidReference<TestObject> ref (registry_);
  EXPECT_FALSE (ref.has_value ());
  EXPECT_EQ (ref.get (), nullptr);
}

TEST_F (TypedUuidReferenceTest, GetReturnsTypedPointer)
{
  auto ref = utils::create_object<TestObject> (
    registry_, TestUuid{ QUuid::createUuid () }, "typed");
  auto * ptr = ref.get ();
  EXPECT_NE (ptr, nullptr);
  EXPECT_EQ (ptr->name (), "typed");
}

TEST_F (TypedUuidReferenceTest, GetOrThrowThrowsWhenUnengaged)
{
  TypedUuidReference<TestObject> ref (registry_);
  EXPECT_THROW (ref.get_or_throw (), std::runtime_error);
}

TEST_F (TypedUuidReferenceTest, GetOrThrowReturnsTypedPointer)
{
  auto ref = utils::create_object<TestObject> (
    registry_, TestUuid{ QUuid::createUuid () }, "throw_test");
  EXPECT_EQ (ref.get_or_throw ()->name (), "throw_test");
}

TEST_F (TypedUuidReferenceTest, IdReturnsTypedUuid)
{
  auto uuid = TestUuid{ QUuid::createUuid () };
  auto ref = utils::create_object<TestObject> (registry_, uuid, "id_test");
  EXPECT_EQ (ref.id (), uuid);
}

TEST_F (TypedUuidReferenceTest, AsUntypedReturnsUnderlyingReference)
{
  auto ref = utils::create_object<TestObject> (
    registry_, TestUuid{ QUuid::createUuid () }, "untyped");
  const auto &untyped = ref.as_untyped ();
  EXPECT_TRUE (untyped.has_value ());
}

TEST_F (TypedUuidReferenceTest, CopySharesReference)
{
  auto ref1 = utils::create_object<TestObject> (
    registry_, TestUuid{ QUuid::createUuid () }, "copy");
  auto id = type_safe::get (ref1.id ());
  EXPECT_EQ (registry_.ref_count (id), 1);

  {
    auto ref2 = ref1;
    EXPECT_EQ (registry_.ref_count (id), 2);
    EXPECT_EQ (ref2.get ()->name (), "copy");
  }

  EXPECT_EQ (registry_.ref_count (id), 1);
}

TEST_F (TypedUuidReferenceTest, MoveTransfersOwnership)
{
  auto ref1 = utils::create_object<TestObject> (
    registry_, TestUuid{ QUuid::createUuid () }, "move");
  auto id = type_safe::get (ref1.id ());
  EXPECT_EQ (registry_.ref_count (id), 1);

  auto ref2 = std::move (ref1);
  EXPECT_FALSE (ref1.has_value ());
  EXPECT_TRUE (ref2.has_value ());
  EXPECT_EQ (registry_.ref_count (id), 1);
  EXPECT_EQ (ref2.get ()->name (), "move");
}

TEST_F (TypedUuidReferenceTest, SerializationRoundTrip)
{
  auto uuid = TestUuid{ QUuid::createUuid () };
  auto ref1 = utils::create_object<TestObject> (registry_, uuid, "ser");
  auto id = type_safe::get (uuid);

  nlohmann::json j;
  to_json (j, ref1);

  TypedUuidReference<TestObject> ref2 (registry_);
  from_json (j, ref2);
  EXPECT_TRUE (ref2.has_value ());
  EXPECT_EQ (ref2.id (), uuid);
  EXPECT_EQ (registry_.ref_count (id), 2);
}

TEST_F (TypedUuidReferenceTest, SetIdForDeferredInit)
{
  auto   uuid = TestUuid{ QUuid::createUuid () };
  auto * obj = new TestObject (uuid, "deferred");
  registry_.register_object (*obj);

  TypedUuidReference<TestObject> ref (registry_);
  EXPECT_FALSE (ref.has_value ());
  ref.set_id (uuid);
  EXPECT_TRUE (ref.has_value ());
  EXPECT_EQ (ref.get ()->name (), "deferred");
}

TEST_F (TypedUuidReferenceTest, GetRegistryReturnsReference)
{
  auto ref = utils::create_object<TestObject> (
    registry_, TestUuid{ QUuid::createUuid () }, "registry");
  auto &reg = ref.get_registry ();
  EXPECT_EQ (&reg, &registry_);
}

// ============================================================================
// registry_object_factory tests
// ============================================================================

class RegistryObjectFactoryTest : public ::testing::Test
{
protected:
  utils::ObjectRegistry registry_;
};

TEST_F (RegistryObjectFactoryTest, CreateObjectRegistersAndReturnsRef)
{
  auto uuid = TestUuid{ QUuid::createUuid () };
  auto ref = utils::create_object<TestObject> (registry_, uuid, "factory");

  EXPECT_TRUE (registry_.contains (type_safe::get (uuid)));
  EXPECT_EQ (registry_.count_matching<TestObject> (), 1);
  EXPECT_EQ (registry_.ref_count (type_safe::get (uuid)), 1);
  EXPECT_EQ (ref.get ()->name (), "factory");
  EXPECT_EQ (ref.id (), uuid);
}

TEST_F (RegistryObjectFactoryTest, CreateObjectSetsParent)
{
  auto ref = utils::create_object<TestObject> (
    registry_, TestUuid{ QUuid::createUuid () }, "parent_test");
  EXPECT_EQ (ref.get ()->parent (), &registry_);
}

TEST_F (RegistryObjectFactoryTest, CreateObjectDuplicateUuidThrows)
{
  auto uuid = TestUuid{ QUuid::createUuid () };
  auto ref1 = utils::create_object<TestObject> (registry_, uuid, "first");
  EXPECT_THROW (
    (void) utils::create_object<TestObject> (registry_, uuid, "dup"),
    std::runtime_error);
}

TEST_F (RegistryObjectFactoryTest, CloneObjectCreatesNewIdentity)
{
  auto ref = utils::create_object<TestObject> (
    registry_, TestUuid{ QUuid::createUuid () }, "original");
  auto cloned_ref = utils::clone_object<TestObject> (
    *ref.get (), registry_, ObjectCloneType::NewIdentity);

  EXPECT_NE (cloned_ref.id (), ref.id ());
  EXPECT_EQ (cloned_ref.get ()->name (), "original");
  EXPECT_EQ (registry_.count_matching<TestObject> (), 2);
}

TEST_F (RegistryObjectFactoryTest, MultipleObjectsCoexist)
{
  auto ref1 = utils::create_object<TestObject> (
    registry_, TestUuid{ QUuid::createUuid () }, "obj1");
  auto ref2 = utils::create_object<TestObject> (
    registry_, TestUuid{ QUuid::createUuid () }, "obj2");
  auto ref3 = utils::create_object<TestObject> (
    registry_, TestUuid{ QUuid::createUuid () }, "obj3");

  EXPECT_EQ (registry_.count_matching<TestObject> (), 3);
  EXPECT_TRUE (registry_.contains (type_safe::get (ref1.id ())));
  EXPECT_TRUE (registry_.contains (type_safe::get (ref2.id ())));
  EXPECT_TRUE (registry_.contains (type_safe::get (ref3.id ())));
}

// ============================================================================
// Destruction order tests (port from old test suite)
// ============================================================================

/**
 * @brief Tests that registry destruction handles objects containing
 *        TypedUuidReferences correctly when external references are held.
 */
class ObjectRegistryDestructionWithExternalRefsTest
    : public ::testing::TestWithParam<bool>
{
};

TEST_P (ObjectRegistryDestructionWithExternalRefsTest, HandlesBothCreationOrders)
{
  const bool create_referenced_first = GetParam ();

  {
    utils::ObjectRegistry registry;

    TypedUuidReference<ContainerTestObject> container_ref{ registry };
    TypedUuidReference<ContainerTestObject> referenced_ref{ registry };
    if (create_referenced_first)
      {
        referenced_ref = utils::create_object<ContainerTestObject> (
          registry, ContainerTestObject::Uuid{ QUuid::createUuid () },
          "Referenced");
        container_ref = utils::create_object<ContainerTestObject> (
          registry, ContainerTestObject::Uuid{ QUuid::createUuid () },
          "Container");
      }
    else
      {
        container_ref = utils::create_object<ContainerTestObject> (
          registry, ContainerTestObject::Uuid{ QUuid::createUuid () },
          "Container");
        referenced_ref = utils::create_object<ContainerTestObject> (
          registry, ContainerTestObject::Uuid{ QUuid::createUuid () },
          "Referenced");
      }

    container_ref.get ()->set_contained_ref (referenced_ref);
  }

  SUCCEED () << "Registry destruction completed without use-after-free";
}

INSTANTIATE_TEST_SUITE_P (
  ObjectRegistryDestruction,
  ObjectRegistryDestructionWithExternalRefsTest,
  ::testing::Bool (),
  [] (const ::testing::TestParamInfo<bool> &param_info) {
    return param_info.param ? "ReferencedFirst" : "ContainerFirst";
  });

/**
 * @brief Tests destruction when objects are deleted via Qt parent-child,
 *        not via ref counting. Regression test for heap-use-after-free.
 */
class ObjectRegistryDestructionViaQtParentChildTest
    : public ::testing::TestWithParam<bool>
{
};

TEST_P (
  ObjectRegistryDestructionViaQtParentChildTest,
  HandlesBothRegistrationOrders)
{
  const bool register_referenced_first = GetParam ();

  {
    utils::ObjectRegistry registry;

    auto * container_obj = new ContainerTestObject (
      ContainerTestObject::Uuid{ QUuid::createUuid () }, "Container");
    auto * referenced_obj = new ContainerTestObject (
      ContainerTestObject::Uuid{ QUuid::createUuid () }, "Referenced");

    if (register_referenced_first)
      {
        registry.register_object (*referenced_obj);
        registry.register_object (*container_obj);
      }
    else
      {
        registry.register_object (*container_obj);
        registry.register_object (*referenced_obj);
      }

    // Create a reference to the referenced object and store it in the
    // container. This increments the referenced object's ref_count to 1.
    {
      TypedUuidReference<ContainerTestObject> ref (
        referenced_obj->get_uuid (), registry);
      container_obj->set_contained_ref (ref);
      // ref goes out of scope here, but the copy inside container_obj keeps
      // ref_count = 1
    }
  }

  SUCCEED () << "Qt parent-child destruction completed without use-after-free";
}

INSTANTIATE_TEST_SUITE_P (
  ObjectRegistryDestruction,
  ObjectRegistryDestructionViaQtParentChildTest,
  ::testing::Bool (),
  [] (const ::testing::TestParamInfo<bool> &param_info) {
    return param_info.param ? "ReferencedFirst" : "ContainerFirst";
  });

TEST (GetTypedTest, ReturnsCorrectObject)
{
  utils::ObjectRegistry registry;
  auto                  ref = utils::create_object<TestObject> (
    registry, TestUuid{ QUuid::createUuid () }, "typed");
  auto &obj = utils::get_typed<TestObject> (registry, ref.id ());
  EXPECT_EQ (obj.name (), "typed");
}

TEST (TypedUuidReferenceConversionTest, DerivedToBaseConversion)
{
  utils::ObjectRegistry registry;
  auto                  derived_ref = utils::create_object<DerivedTestObject> (
    registry, DerivedTestObject::Uuid{ QUuid::createUuid () }, "derived");
  TypedUuidReference<TestObject> base_ref = derived_ref;
  EXPECT_EQ (base_ref.id (), derived_ref.id ());
  EXPECT_EQ (base_ref.get ()->name (), "derived");
}

TEST (TypedUuidReferenceConversionTest, DerivedToBaseSharesRefcount)
{
  utils::ObjectRegistry registry;
  auto                  derived_ref = utils::create_object<DerivedTestObject> (
    registry, DerivedTestObject::Uuid{ QUuid::createUuid () }, "derived");
  const auto raw_id = type_safe::get (derived_ref.id ());
  EXPECT_EQ (registry.ref_count (raw_id), 1);
  {
    TypedUuidReference<TestObject> base_ref = derived_ref;
    EXPECT_EQ (registry.ref_count (raw_id), 2);
  }
  EXPECT_EQ (registry.ref_count (raw_id), 1);
}

} // namespace zrythm::utils
