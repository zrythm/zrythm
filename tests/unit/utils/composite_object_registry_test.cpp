// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>
#include <stdexcept>

#include "utils/composite_object_registry.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "test_uuid_identifiable_qobjects.h"
#include <gtest/gtest.h>

namespace zrythm::utils
{

class CompositeObjectRegistryTest : public ::testing::Test
{
protected:
  void TearDown () override
  {
    // release the references while the registries they point into are
    // still alive
    primary_refs_.clear ();
    secondary_refs_.clear ();
  }

  DerivedTestObject * add_to_primary (TestUuid id, std::string name)
  {
    auto ref = create_object<DerivedTestObject> (primary_, id, std::move (name));
    primary_refs_.push_back (ref.as_untyped ());
    return ref.get ();
  }

  DerivedTestObject * add_to_secondary (TestUuid id, std::string name)
  {
    auto ref =
      create_object<DerivedTestObject> (secondary_, id, std::move (name));
    secondary_refs_.push_back (ref.as_untyped ());
    return ref.get ();
  }

  static QUuid raw (TestUuid id) { return type_safe::get (id); }

  ObjectRegistry             primary_;
  ObjectRegistry             secondary_;
  CompositeObjectRegistry    composite_{ primary_, secondary_ };
  std::vector<UuidReference> primary_refs_;
  std::vector<UuidReference> secondary_refs_;
};

TEST_F (CompositeObjectRegistryTest, ResolvesObjectsFromBothRegistries)
{
  const auto primary_id = TestUuid{ QUuid::createUuid () };
  const auto secondary_id = TestUuid{ QUuid::createUuid () };
  auto *     primary_obj = add_to_primary (primary_id, "primary");
  auto *     secondary_obj = add_to_secondary (secondary_id, "secondary");

  EXPECT_EQ (composite_.find_by_raw_uuid (raw (primary_id)), primary_obj);
  EXPECT_EQ (composite_.find_by_raw_uuid (raw (secondary_id)), secondary_obj);
  EXPECT_TRUE (composite_.contains (raw (primary_id)));
  EXPECT_TRUE (composite_.contains (raw (secondary_id)));
  EXPECT_EQ (composite_.find_by_raw_uuid (QUuid::createUuid ()), nullptr);
}

// The same UUID in both constituents resolves to the primary's object
TEST_F (CompositeObjectRegistryTest, LookupPrefersPrimary)
{
  const auto shared_id = TestUuid{ QUuid::createUuid () };
  auto *     primary_obj = add_to_primary (shared_id, "primary");
  add_to_secondary (shared_id, "secondary");

  EXPECT_EQ (composite_.find_by_raw_uuid (raw (shared_id)), primary_obj);
}

// Objects register in the registry that owns them; the composite is a
// resolution facade and refuses registrations
TEST_F (CompositeObjectRegistryTest, RegistrationIsRefused)
{
  auto obj = std::make_unique<DerivedTestObject> (
    TestUuid{ QUuid::createUuid () }, std::string ("via"));
  EXPECT_THROW (composite_.register_object (*obj), std::runtime_error);
}

// Reference counting routes to the registry owning the UUID
TEST_F (CompositeObjectRegistryTest, ReferenceCountingRoutesToOwner)
{
  const auto id = TestUuid{ QUuid::createUuid () };
  add_to_secondary (id, "secondary");
  EXPECT_EQ (secondary_.ref_count (raw (id)), 1); // the stored reference

  composite_.acquire_reference (raw (id));
  EXPECT_EQ (secondary_.ref_count (raw (id)), 2);
  composite_.release_reference (raw (id));
  EXPECT_EQ (secondary_.ref_count (raw (id)), 1);

  // dropping the stored reference deletes the object through its owner
  secondary_refs_.clear ();
  EXPECT_EQ (composite_.find_by_raw_uuid (raw (id)), nullptr);
}

TEST_F (CompositeObjectRegistryTest, AcquireUnknownUuidThrows)
{
  EXPECT_THROW (
    composite_.acquire_reference (QUuid::createUuid ()), std::runtime_error);
}

// Iteration visits both constituents
TEST_F (CompositeObjectRegistryTest, IterationUnionsBothRegistries)
{
  add_to_primary (TestUuid{ QUuid::createUuid () }, "one");
  add_to_primary (TestUuid{ QUuid::createUuid () }, "two");
  add_to_secondary (TestUuid{ QUuid::createUuid () }, "three");

  EXPECT_EQ (composite_.count_matching<DerivedTestObject> (), 3);
  EXPECT_EQ (primary_.count_matching<DerivedTestObject> (), 2);
  EXPECT_EQ (secondary_.count_matching<DerivedTestObject> (), 1);
}

} // namespace zrythm::utils
