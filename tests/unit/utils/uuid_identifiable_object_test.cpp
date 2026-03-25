// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/uuid_identifiable_object.h"

#include "./uuid_identifiable_object_test.h"
#include "gmock/gmock.h"

namespace zrythm::utils
{

TEST (UuidIdentifiableObjectTest, Creation)
{
  BaseTestObject obj1;
  BaseTestObject obj2;

  // Each object should get a unique UUID
  EXPECT_NE (obj1.get_uuid (), obj2.get_uuid ());
  EXPECT_FALSE (obj1.get_uuid ().is_null ());
  EXPECT_FALSE (obj2.get_uuid ().is_null ());
}

TEST (UuidIdentifiableObjectTest, Serialization)
{
  BaseTestObject obj1;
  nlohmann::json json = obj1;

  auto obj2 = json.get<BaseTestObject> ();

  EXPECT_EQ (obj2.get_uuid (), obj1.get_uuid ());
}

TEST (UuidIdentifiableObjectTest, CopyAndMove)
{
  BaseTestObject obj1;
  auto           id = obj1.get_uuid ();

  // Test copy
  BaseTestObject obj2 (obj1);
  EXPECT_EQ (obj2.get_uuid (), id);

  // Test move
  BaseTestObject obj3 (std::move (obj2));
  EXPECT_EQ (obj3.get_uuid (), id);

  // Test copy assignment
  BaseTestObject obj4;
  obj4 = obj1;
  EXPECT_EQ (obj4.get_uuid (), id);

  // Test move assignment
  BaseTestObject obj5;
  obj5 = std::move (obj3);
  EXPECT_EQ (obj5.get_uuid (), id);
}

TEST (UuidIdentifiableObjectTest, UuidTypeOperations)
{
  TestUuid null_uuid;
  EXPECT_TRUE (null_uuid.is_null ());

  TestUuid uuid1{ QUuid::createUuid () };
  TestUuid uuid2{ QUuid::createUuid () };
  TestUuid uuid1_copy{ uuid1 };

  // Comparison operators
  EXPECT_EQ (uuid1, uuid1_copy);
  EXPECT_NE (uuid1, uuid2);
}

TEST (UuidIdentifiableObjectTest, BoostDescribeFormatter)
{
  // Test BaseTestObject formatting
  BaseTestObject baseObj;
  std::string    baseStr = fmt::format ("{}", baseObj);
  EXPECT_THAT (baseStr, testing::StartsWith ("{ { .uuid_="));
  EXPECT_THAT (baseStr, testing::EndsWith ("}"));

  // Test DerivedTestObject formatting
  DerivedTestObject derivedObj (TestUuid{ QUuid::createUuid () }, "TestObject");
  std::string       derivedStr = fmt::format ("{}", derivedObj);
  EXPECT_THAT (derivedStr, testing::StartsWith ("{ { { .uuid_="));
  EXPECT_THAT (derivedStr, testing::HasSubstr (".name_=TestObject"));
  EXPECT_THAT (derivedStr, testing::EndsWith ("}"));
}

TEST (UuidIdentifiableObjectTest, UuidTypeFormatter)
{
  TestUuid    uuid{ QUuid::createUuid () };
  std::string uuidStr = fmt::format ("{}", uuid);
  EXPECT_EQ (
    uuidStr,
    type_safe::get (uuid).toString (QUuid::WithoutBraces).toStdString ());
}

TEST (UuidIdentifiableObjectTest, UuidReferenceSerialization)
{
  using TestRegistry = UuidIdentifiableObjectRegistryTest::TestRegistry;

  auto * obj =
    new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "RefTest");
  TestRegistry registry;
  registry.register_object (obj);

  // Serialize UUID reference
  utils::UuidReference<TestRegistry> ref (obj->get_uuid (), registry);
  nlohmann::json                     j;
  to_json (j, ref);

  // Create new reference from JSON
  utils::UuidReference<TestRegistry> new_ref (registry);
  from_json (j, new_ref);

  // Verify same object
  EXPECT_EQ (new_ref.id (), ref.id ());
  EXPECT_EQ (new_ref.get_object_as<DerivedTestObject> ()->name (), "RefTest");
}

TEST (UuidIdentifiableObjectTest, UuidReferenceFormatter)
{
  using TestRegistry = UuidIdentifiableObjectRegistryTest::TestRegistry;

  auto * obj =
    new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "RefTest");
  TestRegistry registry;
  registry.register_object (obj);

  utils::UuidReference<TestRegistry> ref (obj->get_uuid (), registry);
  std::string                        refStr = fmt::format ("{}", ref);

  // Expected format: { .id_=<uuid>, .object=<formatted_object> }
  std::string expectedUuid =
    type_safe::get (obj->get_uuid ())
      .toString (QUuid::WithoutBraces)
      .toStdString ();

  // Verify the format includes both UUID and object
  EXPECT_THAT (refStr, testing::StartsWith ("{ .id_=" + expectedUuid));
  EXPECT_THAT (refStr, testing::HasSubstr (", .object="));
  EXPECT_THAT (refStr, testing::HasSubstr (".name_=RefTest"));
  EXPECT_THAT (refStr, testing::EndsWith (" }"));
}

TEST_F (UuidIdentifiableObjectRegistryTest, BasicRegistration)
{
  EXPECT_EQ (registry_.size (), 3);
  EXPECT_TRUE (registry_.contains (obj1_->get_uuid ()));
  EXPECT_FALSE (registry_.contains (TestUuid{}));
}

TEST_F (UuidIdentifiableObjectRegistryTest, DuplicateRejection)
{
  auto dupObj =
    std::make_unique<DerivedTestObject> (obj1_->get_uuid (), "Duplicate");
  EXPECT_THROW (registry_.register_object (dupObj.get ()), std::runtime_error);
}

TEST_F (UuidIdentifiableObjectRegistryTest, ObjectLookup)
{
  auto found_var = registry_.find_by_id_or_throw (obj2_->get_uuid ());
  std::visit (
    [&] (auto &&found) { EXPECT_EQ (found->name (), "Object2"); }, found_var);
  EXPECT_THROW (registry_.find_by_id_or_throw (TestUuid{}), std::runtime_error);
}

TEST_F (UuidIdentifiableObjectRegistryTest, ViewIteration)
{
  std::vector<TestUuid> uuids{ obj3_->get_uuid (), obj1_->get_uuid () };
  auto span = utils::UuidIdentifiableObjectView<TestRegistry> (registry_, uuids);

  // Test iteration order and content
  std::vector<std::string> names;
  for (const auto &obj : span)
    {
      names.push_back (std::visit ([] (auto * o) { return o->name (); }, obj));
    }

  EXPECT_EQ (names.size (), 2);
  EXPECT_EQ (names[0], "Object3");
  EXPECT_EQ (names[1], "Object1");
}

TEST_F (UuidIdentifiableObjectRegistryTest, UuidListRetrieval)
{
  const auto uuids = registry_.get_uuids ();
  EXPECT_EQ (uuids.size (), 3);
  EXPECT_TRUE (std::ranges::find (uuids, obj2_->get_uuid ()) != uuids.end ());
}

TEST_F (UuidIdentifiableObjectRegistryTest, ViewAccessors)
{
  std::vector<TestUuid> uuids{ obj2_->get_uuid (), obj3_->get_uuid () };
  auto span = utils::UuidIdentifiableObjectView<TestRegistry> (registry_, uuids);

  BOOST_STL_INTERFACES_STATIC_ASSERT_CONCEPT (
    utils::UuidIdentifiableObjectView<TestRegistry>::Iterator,
    std::random_access_iterator)

  EXPECT_FALSE (span.empty ());
  EXPECT_EQ (span.size (), 2);
  EXPECT_EQ (
    std::visit ([&] (auto &&obj) { return obj->name (); }, span[0]), "Object2");
  EXPECT_EQ (
    std::visit ([&] (auto &&obj) { return obj->name (); }, span.back ()),
    "Object3");
}

TEST_F (UuidIdentifiableObjectRegistryTest, ReferenceCountingWithCopies)
{
  auto ref = std::make_optional (registry_.create_object<DerivedTestObject> (
    TestUuid{ QUuid::createUuid () }, "ReferenceTest"));
  const auto id = ref->id ();

  EXPECT_TRUE (registry_.contains (id));
  EXPECT_EQ (registry_.size (), 4);
  EXPECT_EQ (registry_.reference_count (id), 1);

  {
    auto ref2 = *ref; // Copy increases ref count
    EXPECT_TRUE (registry_.contains (id));
    EXPECT_EQ (registry_.reference_count (id), 2);
  } // ref2 destroyed - decrease ref count

  // Should still exist after partial release
  EXPECT_TRUE (registry_.contains (id));
  EXPECT_EQ (registry_.reference_count (id), 1);

  ref.reset (); // Release final reference
  EXPECT_FALSE (registry_.contains (id));
  EXPECT_EQ (registry_.size (), 3);
}

TEST_F (UuidIdentifiableObjectRegistryTest, ReferenceCountingWithMoves)
{
  // Create initial reference
  auto ref = registry_.create_object<DerivedTestObject> (
    TestUuid{ QUuid::createUuid () }, "MoveTest");
  auto id = ref.id ();

  EXPECT_EQ (registry_.reference_count (id), 1);

  // Test move construction
  {
    auto moved_ref (std::move (ref));
    EXPECT_TRUE (registry_.contains (id));
    EXPECT_EQ (registry_.size (), 4);
    EXPECT_EQ (registry_.reference_count (id), 1); // Count shouldn't change
  } // moved_ref destroyed - should release reference

  EXPECT_FALSE (registry_.contains (id));
  EXPECT_EQ (registry_.size (), 3);

  // Test move assignment
  ref = registry_.create_object<DerivedTestObject> (
    TestUuid{ QUuid::createUuid () }, "MoveTest2");
  id = ref.id ();
  EXPECT_EQ (registry_.reference_count (id), 1);

  {
    auto moved_ref = std::move (ref);
    EXPECT_TRUE (registry_.contains (id));
    EXPECT_EQ (registry_.size (), 4);
    EXPECT_EQ (registry_.reference_count (id), 1); // Count shouldn't change
  } // moved_ref destroyed - should release reference

  EXPECT_FALSE (registry_.contains (id));
  EXPECT_EQ (registry_.size (), 3);
}

TEST_F (UuidIdentifiableObjectRegistryTest, ReferenceCountingWithSerialization)
{
  // Create initial reference
  auto ref = registry_.create_object<DerivedTestObject> (
    TestUuid{ QUuid::createUuid () }, "SerializationTest");
  auto id = ref.id ();

  EXPECT_EQ (registry_.reference_count (id), 1);
  EXPECT_EQ (registry_.size (), 4);

  // Serialize to json
  nlohmann::json j;
  to_json (j, ref);

  {
    // Create new instance
    auto new_ref = registry_.create_object<DerivedTestObject> (
      TestUuid{ QUuid::createUuid () }, "SerializationTest2");
    auto new_id = new_ref.id ();
    EXPECT_TRUE (registry_.contains (new_id));
    EXPECT_EQ (registry_.reference_count (id), 1);
    EXPECT_EQ (registry_.reference_count (new_id), 1);
    EXPECT_EQ (registry_.size (), 5);

    // Deserialize into new instance and verify temporary ID is gone
    from_json (j, new_ref);
    EXPECT_NE (new_ref.id (), new_id);
    EXPECT_EQ (new_ref.id (), id);
    EXPECT_EQ (registry_.reference_count (id), 2);
    EXPECT_FALSE (registry_.contains (new_id));
    EXPECT_EQ (registry_.size (), 4);
  }

  // Verify temporary ref is gone
  EXPECT_EQ (registry_.reference_count (id), 1);
  EXPECT_EQ (registry_.size (), 4);
}

TEST_F (UuidIdentifiableObjectRegistryTest, ObjectParentManagement)
{
  auto * obj =
    new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "Orphan");
  EXPECT_EQ (obj->parent (), nullptr);

  registry_.register_object (obj);
  EXPECT_EQ (obj->parent (), &registry_);

  // LeakSanitizer should complain if the object still exists when tests finishes
}

TEST_F (UuidIdentifiableObjectRegistryTest, ViewEdgeCases)
{
  // Empty span
  std::vector<TestUuid> empty;
  auto                  empty_span =
    utils::UuidIdentifiableObjectView<TestRegistry> (registry_, empty);
  EXPECT_TRUE (empty_span.empty ());

  // Invalid access
  std::vector<TestUuid> single{ obj1_->get_uuid () };
  auto                  span =
    utils::UuidIdentifiableObjectView<TestRegistry> (registry_, single);
  EXPECT_THROW (span.at (1), std::out_of_range);
}

TEST_F (UuidIdentifiableObjectRegistryTest, ViewWithUuidReferences)
{
  // Create vector of UuidReferences
  std::vector<utils::UuidReference<TestRegistry>> refs;
  refs.emplace_back (obj1_->get_uuid (), registry_);
  refs.emplace_back (obj2_->get_uuid (), registry_);
  refs.emplace_back (obj3_->get_uuid (), registry_);

  // Create compatible span
  utils::UuidIdentifiableObjectView<TestRegistry> compatible_span (refs);

  // Verify basic span properties
  EXPECT_EQ (compatible_span.size (), 3);
  EXPECT_FALSE (compatible_span.empty ());

  // Verify object access through span
  std::vector<DerivedTestObject *> objects;
  for (const auto &var : compatible_span)
    {
      auto * obj = std::visit ([] (auto * o) { return o; }, var);
      objects.push_back (obj);
    }

  // Should contain all test objects in original order
  EXPECT_THAT (objects, testing::ElementsAre (obj1_, obj2_, obj3_));

  // Test UUID projections
  auto uuid_projection = [] (const auto &var) {
    return std::visit ([] (auto * obj) { return obj->get_uuid (); }, var);
  };
  std::vector<TestUuid> span_uuids;
  std::ranges::transform (
    compatible_span, std::back_inserter (span_uuids), uuid_projection);

  EXPECT_THAT (
    span_uuids,
    testing::UnorderedElementsAre (
      obj1_->get_uuid (), obj2_->get_uuid (), obj3_->get_uuid ()));

  // Test type filtering
  auto derived_ptrs =
    compatible_span.template get_elements_by_type<DerivedTestObject> ();
  EXPECT_EQ (std::ranges::distance (derived_ptrs), 3);
  for (auto * ptr : derived_ptrs)
    {
      EXPECT_THAT (ptr, testing::AnyOf (obj1_, obj2_, obj3_));
    }

  // Test base type access
  auto base_ptrs = compatible_span.as_base_type ();
  EXPECT_EQ (base_ptrs.size (), 3);
  for (auto * base_ptr : base_ptrs)
    {
      EXPECT_NE (dynamic_cast<DerivedTestObject *> (base_ptr), nullptr);
    }
}

TEST_F (UuidIdentifiableObjectRegistryTest, ObjectCreation)
{
  auto ref = registry_.create_object<DerivedTestObject> (
    TestUuid{ QUuid::createUuid () }, "FactoryMade");
  auto found = registry_.find_by_id (ref.id ());
  EXPECT_TRUE (found.has_value ());
}

TEST_F (UuidIdentifiableObjectRegistryTest, ObjectCloning)
{
  auto clone_ref = registry_.clone_object (
    *obj1_, TestUuid{ QUuid::createUuid () }, "ClonedObject");
  EXPECT_NE (clone_ref.id (), obj1_->get_uuid ());
  EXPECT_EQ (registry_.size (), 4);
}

void
from_json (
  nlohmann::json                                   &j,
  UuidIdentifiableObjectRegistryTest::TestRegistry &obj)
{
  auto builder = TestObjectBuilder ().with_int_dependency (42);
  from_json_with_builder (j, obj, builder);
}

TEST_F (UuidIdentifiableObjectRegistryTest, RegistrySerialization)
{
  // Serialize the registry
  nlohmann::json json = registry_;

  // Create a new registry and deserialize into it
  TestRegistry new_registry;
  from_json (json, new_registry);

  // Verify all objects were deserialized correctly
  EXPECT_EQ (new_registry.size (), 3);
  EXPECT_TRUE (new_registry.contains (obj1_->get_uuid ()));
  EXPECT_TRUE (new_registry.contains (obj2_->get_uuid ()));
  EXPECT_TRUE (new_registry.contains (obj3_->get_uuid ()));

  // Verify object data was preserved
  auto obj1_var = new_registry.find_by_id_or_throw (obj1_->get_uuid ());
  std::visit (
    [] (auto * obj) { EXPECT_EQ (obj->name (), "Object1"); }, obj1_var);
}

/**
 * @brief Tests that registry destruction handles objects containing
 *        UuidReferences correctly when external references are held.
 *
 * When UuidReference objects are held externally, objects are deleted via
 * reference counting before the registry destructor runs. This test verifies
 * that this scenario works correctly.
 */
class DestructionWithExternalReferencesTest
    : public ::testing::TestWithParam<bool>
{
};

TEST_P (DestructionWithExternalReferencesTest, HandlesBothCreationOrders)
{
  const bool create_referenced_first = GetParam ();
  using ContainerRegistry = ContainerTestObject::ContainerRegistry;

  // Create a registry in a scope so it gets destroyed at the end
  {
    ContainerRegistry registry;

    // Create objects in specified order
    ContainerTestObject::UuidRef container_ref{ registry };
    ContainerTestObject::UuidRef referenced_ref{ registry };
    if (create_referenced_first)
      {
        referenced_ref = registry.create_object<ContainerTestObject> (
          TestUuid{ QUuid::createUuid () }, "Referenced");
        container_ref = registry.create_object<ContainerTestObject> (
          TestUuid{ QUuid::createUuid () }, "Container");
      }
    else
      {
        container_ref = registry.create_object<ContainerTestObject> (
          TestUuid{ QUuid::createUuid () }, "Container");
        referenced_ref = registry.create_object<ContainerTestObject> (
          TestUuid{ QUuid::createUuid () }, "Referenced");
      }

    // Get raw pointer and set up the reference
    auto * container = container_ref.get_object_as<ContainerTestObject> ();
    container->set_contained_ref (referenced_ref);

    // Registry goes out of scope here - this should not cause use-after-free
  }

  // If we get here without ASan errors, the test passes
  SUCCEED () << "Registry destruction completed without use-after-free";
}

INSTANTIATE_TEST_SUITE_P (
  UuidIdentifiableObjectRegistryDestructionTest,
  DestructionWithExternalReferencesTest,
  ::testing::Bool (),
  [] (const ::testing::TestParamInfo<bool> &param_info) {
    return param_info.param ? "ReferencedFirst" : "ContainerFirst";
  });

/**
 * @brief Tests destruction when objects are deleted via Qt parent-child,
 *        not via ref counting.
 *
 * This reproduces the exact scenario from the real code: objects created
 * without external UuidReference holders, so they're deleted via Qt's
 * parent-child mechanism when the registry is destroyed.
 *
 * Regression test for: heap-use-after-free in ~OwningObjectRegistry()
 */
class DestructionViaQtParentChildTest : public ::testing::TestWithParam<bool>
{
};

TEST_P (DestructionViaQtParentChildTest, HandlesBothRegistrationOrders)
{
  const bool register_referenced_first = GetParam ();
  using ContainerRegistry = ContainerTestObject::ContainerRegistry;

  // Create a registry in a scope so it gets destroyed at the end
  {
    ContainerRegistry registry;

    // Create objects directly (not via create_object) so they have no external
    // UuidReference holding them. This ensures they're deleted via Qt's
    // parent-child mechanism, not via ref counting.
    auto * container_obj =
      new ContainerTestObject (TestUuid{ QUuid::createUuid () }, "Container");
    auto * referenced_obj =
      new ContainerTestObject (TestUuid{ QUuid::createUuid () }, "Referenced");

    // Register objects in specified order - this sets the registry as parent
    if (register_referenced_first)
      {
        registry.register_object (referenced_obj);
        registry.register_object (container_obj);
      }
    else
      {
        registry.register_object (container_obj);
        registry.register_object (referenced_obj);
      }

    // Create a UuidReference to the referenced object and store it in the
    // container. This increments the referenced object's ref_count to 1.
    {
      ContainerTestObject::UuidRef ref (referenced_obj->get_uuid (), registry);
      container_obj->set_contained_ref (ref);
      // ref goes out of scope here, but the copy inside container_obj keeps
      // ref_count = 1
    }

    // Registry goes out of scope here - this should not cause use-after-free
  }

  // If we get here without ASan errors, the test passes
  SUCCEED () << "Registry destruction completed without use-after-free";
}

INSTANTIATE_TEST_SUITE_P (
  UuidIdentifiableObjectRegistryDestructionTest,
  DestructionViaQtParentChildTest,
  ::testing::Bool (),
  [] (const ::testing::TestParamInfo<bool> &param_info) {
    return param_info.param ? "ReferencedFirst" : "ContainerFirst";
  });
}
