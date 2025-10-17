// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "structure/arrangement/named_object.h"
#include "utils/utf8_string.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ArrangerObjectNameTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    parent = std::make_unique<MockQObject> ();
    name_obj = std::make_unique<ArrangerObjectName> ();
  }

  std::unique_ptr<MockQObject>        parent;
  std::unique_ptr<ArrangerObjectName> name_obj;
};

// Test initial state
TEST_F (ArrangerObjectNameTest, InitialState)
{
  EXPECT_TRUE (name_obj->name ().isEmpty ());
  EXPECT_TRUE (name_obj->get_name ().empty ());
}

// Test setting and getting name
TEST_F (ArrangerObjectNameTest, SetGetName)
{
  QString test_name ("Test Object");

  // Set name
  name_obj->setName (test_name);

  // Verify
  EXPECT_EQ (name_obj->name (), test_name);
  EXPECT_EQ (name_obj->get_name ().to_qstring (), test_name);

  // Set same name again (should be no-op)
  testing::MockFunction<void (QString)> mockNameChanged;
  QObject::connect (
    name_obj.get (), &ArrangerObjectName::nameChanged, parent.get (),
    mockNameChanged.AsStdFunction ());

  EXPECT_CALL (mockNameChanged, Call (test_name)).Times (0);
  name_obj->setName (test_name);
}

// Test nameChanged signal
TEST_F (ArrangerObjectNameTest, NameChangedSignal)
{
  QString test_name1 ("First Name");
  QString test_name2 ("Second Name");

  // Setup signal watcher
  testing::MockFunction<void (QString)> mockNameChanged;
  QObject::connect (
    name_obj.get (), &ArrangerObjectName::nameChanged, parent.get (),
    mockNameChanged.AsStdFunction ());

  // First set
  EXPECT_CALL (mockNameChanged, Call (test_name1)).Times (1);
  name_obj->setName (test_name1);

  // Change name
  EXPECT_CALL (mockNameChanged, Call (test_name2)).Times (1);
  name_obj->setName (test_name2);
}

// Test serialization/deserialization
TEST_F (ArrangerObjectNameTest, Serialization)
{
  // Set name
  QString test_name ("Serialized Object");
  name_obj->setName (test_name);

  // Serialize
  nlohmann::json j;
  to_json (j, *name_obj);

  // Create new object
  ArrangerObjectName new_name_obj;
  from_json (j, new_name_obj);

  // Verify
  EXPECT_EQ (new_name_obj.name (), test_name);
  EXPECT_EQ (new_name_obj.get_name ().to_qstring (), test_name);

  // Test with empty name
  name_obj->setName ("");
  to_json (j, *name_obj);
  from_json (j, new_name_obj);
  EXPECT_TRUE (new_name_obj.name ().isEmpty ());
}

// Test copying with init_from
TEST_F (ArrangerObjectNameTest, Copying)
{
  // Set name in source
  QString test_name ("Copied Object");
  name_obj->setName (test_name);

  // Create target
  ArrangerObjectName target;

  // Copy
  init_from (target, *name_obj, utils::ObjectCloneType::Snapshot);

  // Verify
  EXPECT_EQ (target.name (), test_name);
  EXPECT_EQ (target.get_name ().to_qstring (), test_name);

  // Test copying empty name
  name_obj->setName ("");
  init_from (target, *name_obj, utils::ObjectCloneType::Snapshot);
  EXPECT_TRUE (target.name ().isEmpty ());
}

// Test Utf8String handling
TEST_F (ArrangerObjectNameTest, Utf8StringHandling)
{
  // Test with special characters
  QString special_chars ("Object 对象名称");
  name_obj->setName (special_chars);
  EXPECT_EQ (name_obj->name (), special_chars);

  // Test with HTML characters
  QString html_chars ("Object <name> & \"quoted\"");
  name_obj->setName (html_chars);
  EXPECT_EQ (name_obj->name (), html_chars);

  // Verify escaped name is generated
  EXPECT_EQ (
    name_obj->get_name ().escape_html ().to_qstring (),
    "Object &lt;name&gt; &amp; &quot;quoted&quot;");
}

// Test edge cases
TEST_F (ArrangerObjectNameTest, EdgeCases)
{
  // Very long name
  QString long_name (1024, 'a'); // 1KB string
  name_obj->setName (long_name);
  EXPECT_EQ (name_obj->name (), long_name);

  // Empty string
  name_obj->setName ("");
  EXPECT_TRUE (name_obj->name ().isEmpty ());

  // Unicode characters
  QString unicode_name ("对象名称 - オブジェクト名 - 객체 이름");
  name_obj->setName (unicode_name);
  EXPECT_EQ (name_obj->name (), unicode_name);

  // Null string
  name_obj->setName (QString ());
  EXPECT_TRUE (name_obj->name ().isEmpty ());
}

} // namespace zrythm::structure::arrangement
