// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/change_qobject_property_command.h"
#include "commands/change_uuid_identifiable_object_property_command.h"
#include "utils/object_registry.h"

#include "helpers/mock_qobject.h"
#include "helpers/mock_uuid_object.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class ChangeUuidIdentifiableObjectPropertyCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    obj_ = new MockUuidObject ();
    registry_.register_object (*obj_);
  }

  /**
   * @brief Returns a MockQObject whose parent is @ref obj_, so its lifetime
   * is tied to the UUID-identifiable parent (for sub-object tests).
   */
  MockQObject * createChildObject () { return new MockQObject (obj_); }

  // Destruction order: obj_ (raw, owned by registry) is cleaned up when
  // commands holding UUID references are destroyed. registry_ is destroyed
  // last.
  utils::ObjectRegistry registry_;
  MockUuidObject *      obj_ = nullptr;
};

TEST_F (ChangeUuidIdentifiableObjectPropertyCommandTest, DirectCaseRedoUndo)
{
  EXPECT_EQ (obj_->property ("intValue").toInt (), 42);

  ChangeUuidIdentifiableObjectPropertyCommand cmd (
    *obj_, registry_, QStringLiteral ("intValue"), 100,
    Command::EngineRequirement::None);

  cmd.redo ();
  EXPECT_EQ (obj_->property ("intValue").toInt (), 100);

  cmd.undo ();
  EXPECT_EQ (obj_->property ("intValue").toInt (), 42);

  cmd.redo ();
  EXPECT_EQ (obj_->property ("intValue").toInt (), 100);
}

TEST_F (ChangeUuidIdentifiableObjectPropertyCommandTest, SubObjectCaseRedoUndo)
{
  auto * child = createChildObject ();

  EXPECT_EQ (child->intValue (), 42);

  ChangeUuidIdentifiableObjectPropertyCommand cmd (
    *obj_, *child, registry_, QStringLiteral ("intValue"), 200,
    Command::EngineRequirement::None);

  cmd.redo ();
  EXPECT_EQ (child->intValue (), 200);

  cmd.undo ();
  EXPECT_EQ (child->intValue (), 42);

  cmd.redo ();
  EXPECT_EQ (child->intValue (), 200);
}

TEST_F (ChangeUuidIdentifiableObjectPropertyCommandTest, MergeWithSameType)
{
  ChangeUuidIdentifiableObjectPropertyCommand cmd1 (
    *obj_, registry_, QStringLiteral ("intValue"), 25,
    Command::EngineRequirement::None);
  ChangeUuidIdentifiableObjectPropertyCommand cmd2 (
    *obj_, registry_, QStringLiteral ("intValue"), 75,
    Command::EngineRequirement::None);

  cmd1.redo ();
  EXPECT_EQ (obj_->property ("intValue").toInt (), 25);

  EXPECT_TRUE (cmd1.merge_with (cmd2));

  cmd1.redo ();
  EXPECT_EQ (obj_->property ("intValue").toInt (), 75);
}

TEST_F (ChangeUuidIdentifiableObjectPropertyCommandTest, DoesNotMergeWithBaseType)
{
  ChangeUuidIdentifiableObjectPropertyCommand uuid_cmd (
    *obj_, registry_, QStringLiteral ("intValue"), 25,
    Command::EngineRequirement::None);
  ChangeQObjectPropertyCommand base_cmd (
    *obj_, QStringLiteral ("intValue"), 75, Command::EngineRequirement::None);

  // The two variants never merge in either direction: merging one into
  // the other would lose the UUID variant's refcount safety
  EXPECT_FALSE (uuid_cmd.merge_with (base_cmd));
  EXPECT_FALSE (base_cmd.merge_with (uuid_cmd));
}

TEST_F (
  ChangeUuidIdentifiableObjectPropertyCommandTest,
  DoesNotMergeWithDifferentObject)
{
  auto * obj2 = new MockUuidObject ();
  registry_.register_object (*obj2);

  ChangeUuidIdentifiableObjectPropertyCommand cmd1 (
    *obj_, registry_, QStringLiteral ("intValue"), 25,
    Command::EngineRequirement::None);
  ChangeUuidIdentifiableObjectPropertyCommand cmd2 (
    *obj2, registry_, QStringLiteral ("intValue"), 75,
    Command::EngineRequirement::None);

  EXPECT_FALSE (cmd1.merge_with (cmd2));
}

} // namespace zrythm::commands
