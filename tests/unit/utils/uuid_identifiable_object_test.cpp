// SPDX-FileCopyrightText: © 2024, 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "utils/format_boost.h"
#include "utils/uuid_identifiable_object.h"

#include "./test_uuid_identifiable_qobjects.h"
#include <gmock/gmock.h>

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

  BaseTestObject obj2;
  from_json (json, obj2);

  EXPECT_EQ (obj2.get_uuid (), obj1.get_uuid ());
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
  EXPECT_THAT (baseStr, testing::StartsWith ("{ { { .uuid_="));
  EXPECT_THAT (baseStr, testing::EndsWith ("}"));

  // Test DerivedTestObject formatting
  DerivedTestObject derivedObj (TestUuid{ QUuid::createUuid () }, "TestObject");
  std::string       derivedStr = fmt::format ("{}", derivedObj);
  EXPECT_THAT (derivedStr, testing::StartsWith ("{ { { { .uuid_="));
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
} // namespace
