// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "structure/arrangement/muteable_object.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ArrangerObjectMuteFunctionalityTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    parent = std::make_unique<MockQObject> ();
    mute_obj = std::make_unique<ArrangerObjectMuteFunctionality> ();
  }

  std::unique_ptr<MockQObject>                     parent;
  std::unique_ptr<ArrangerObjectMuteFunctionality> mute_obj;
};

// Test initial state
TEST_F (ArrangerObjectMuteFunctionalityTest, InitialState)
{
  EXPECT_FALSE (mute_obj->muted ());
}

// Test setting and getting mute state
TEST_F (ArrangerObjectMuteFunctionalityTest, SetGetMuted)
{
  // Set to muted
  mute_obj->setMuted (true);
  EXPECT_TRUE (mute_obj->muted ());

  // Set to unmuted
  mute_obj->setMuted (false);
  EXPECT_FALSE (mute_obj->muted ());

  // Set same state (should be no-op)
  testing::MockFunction<void (bool)> mockMutedChanged;
  QObject::connect (
    mute_obj.get (), &ArrangerObjectMuteFunctionality::mutedChanged,
    parent.get (), mockMutedChanged.AsStdFunction ());

  EXPECT_CALL (mockMutedChanged, Call (false)).Times (0);
  mute_obj->setMuted (false);
}

// Test mutedChanged signal
TEST_F (ArrangerObjectMuteFunctionalityTest, MutedChangedSignal)
{
  // Setup signal watcher
  testing::MockFunction<void (bool)> mockMutedChanged;
  QObject::connect (
    mute_obj.get (), &ArrangerObjectMuteFunctionality::mutedChanged,
    parent.get (), mockMutedChanged.AsStdFunction ());

  // First mute
  EXPECT_CALL (mockMutedChanged, Call (true)).Times (1);
  mute_obj->setMuted (true);

  // Unmute
  EXPECT_CALL (mockMutedChanged, Call (false)).Times (1);
  mute_obj->setMuted (false);
}

// Test serialization/deserialization
TEST_F (ArrangerObjectMuteFunctionalityTest, Serialization)
{
  // Set muted state
  mute_obj->setMuted (true);

  // Serialize
  nlohmann::json j;
  to_json (j, *mute_obj);

  // Create new object
  ArrangerObjectMuteFunctionality new_mute_obj;
  from_json (j, new_mute_obj);

  // Verify
  EXPECT_TRUE (new_mute_obj.muted ());

  // Test with false state
  mute_obj->setMuted (false);
  to_json (j, *mute_obj);
  from_json (j, new_mute_obj);
  EXPECT_FALSE (new_mute_obj.muted ());
}

// Test copying with init_from
TEST_F (ArrangerObjectMuteFunctionalityTest, Copying)
{
  // Set muted state in source
  mute_obj->setMuted (true);

  // Create target
  ArrangerObjectMuteFunctionality target;

  // Copy
  init_from (target, *mute_obj, utils::ObjectCloneType::Snapshot);

  // Verify
  EXPECT_TRUE (target.muted ());

  // Test copying false state
  mute_obj->setMuted (false);
  init_from (target, *mute_obj, utils::ObjectCloneType::Snapshot);
  EXPECT_FALSE (target.muted ());
}

} // namespace zrythm::structure::arrangement
