// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "structure/arrangement/colored_object.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ArrangerObjectColorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    parent = std::make_unique<MockQObject> ();
    color_obj = std::make_unique<ArrangerObjectColor> ();
  }

  std::unique_ptr<MockQObject>         parent;
  std::unique_ptr<ArrangerObjectColor> color_obj;
};

// Test initial state
TEST_F (ArrangerObjectColorTest, InitialState)
{
  EXPECT_FALSE (color_obj->useColor ());
  EXPECT_FALSE (color_obj->color ().isValid ());
}

// Test setting and getting color
TEST_F (ArrangerObjectColorTest, SetGetColor)
{
  QColor test_color (255, 0, 0); // Red

  // Set color
  color_obj->setColor (test_color);

  // Verify
  EXPECT_TRUE (color_obj->useColor ());
  EXPECT_EQ (color_obj->color (), test_color);

  // Set same color again (should be no-op)
  testing::MockFunction<void (QColor)> mockColorChanged;
  QObject::connect (
    color_obj.get (), &ArrangerObjectColor::colorChanged, parent.get (),
    mockColorChanged.AsStdFunction ());

  EXPECT_CALL (mockColorChanged, Call (test_color)).Times (0);
  color_obj->setColor (test_color);
}

// Test unsetting color
TEST_F (ArrangerObjectColorTest, UnsetColor)
{
  // First set a color
  QColor test_color (0, 255, 0); // Green
  color_obj->setColor (test_color);

  // Verify initial state
  EXPECT_TRUE (color_obj->useColor ());

  // Setup signal watcher
  testing::MockFunction<void (bool)> mockUseColorChanged;
  QObject::connect (
    color_obj.get (), &ArrangerObjectColor::useColorChanged, parent.get (),
    mockUseColorChanged.AsStdFunction ());

  // Unset color
  EXPECT_CALL (mockUseColorChanged, Call (false)).Times (1);
  color_obj->unsetColor ();

  // Verify
  EXPECT_FALSE (color_obj->useColor ());
  EXPECT_FALSE (color_obj->color ().isValid ());
}

// Test colorChanged signal
TEST_F (ArrangerObjectColorTest, ColorChangedSignal)
{
  QColor test_color1 (0, 0, 255);   // Blue
  QColor test_color2 (255, 255, 0); // Yellow

  // Setup signal watcher
  testing::MockFunction<void (QColor)> mockColorChanged;
  QObject::connect (
    color_obj.get (), &ArrangerObjectColor::colorChanged, parent.get (),
    mockColorChanged.AsStdFunction ());

  // First set
  EXPECT_CALL (mockColorChanged, Call (test_color1)).Times (1);
  color_obj->setColor (test_color1);

  // Change color
  EXPECT_CALL (mockColorChanged, Call (test_color2)).Times (1);
  color_obj->setColor (test_color2);
}

// Test useColorChanged signal
TEST_F (ArrangerObjectColorTest, UseColorChangedSignal)
{
  QColor test_color (128, 0, 128); // Purple

  // Setup signal watcher
  testing::MockFunction<void (bool)> mockUseColorChanged;
  QObject::connect (
    color_obj.get (), &ArrangerObjectColor::useColorChanged, parent.get (),
    mockUseColorChanged.AsStdFunction ());

  // Set color (should enable useColor)
  EXPECT_CALL (mockUseColorChanged, Call (true)).Times (1);
  color_obj->setColor (test_color);

  // Unset color (should disable useColor)
  EXPECT_CALL (mockUseColorChanged, Call (false)).Times (1);
  color_obj->unsetColor ();
}

// Test serialization/deserialization
TEST_F (ArrangerObjectColorTest, Serialization)
{
  // Set color
  QColor test_color (255, 165, 0); // Orange
  color_obj->setColor (test_color);

  // Serialize
  nlohmann::json j;
  to_json (j, *color_obj);

  // Create new object
  ArrangerObjectColor new_color_obj;
  from_json (j, new_color_obj);

  // Verify
  EXPECT_TRUE (new_color_obj.useColor ());
  EXPECT_EQ (new_color_obj.color (), test_color);

  // Test with no color
  color_obj->unsetColor ();
  to_json (j, *color_obj);
  from_json (j, new_color_obj);
  EXPECT_FALSE (new_color_obj.useColor ());
}

// Test copying with init_from
TEST_F (ArrangerObjectColorTest, Copying)
{
  // Set color in source
  QColor test_color (75, 0, 130); // Indigo
  color_obj->setColor (test_color);

  // Create target
  ArrangerObjectColor target;

  // Copy
  init_from (target, *color_obj, utils::ObjectCloneType::Snapshot);

  // Verify
  EXPECT_TRUE (target.useColor ());
  EXPECT_EQ (target.color (), test_color);

  // Test copying when no color is set
  color_obj->unsetColor ();
  init_from (target, *color_obj, utils::ObjectCloneType::Snapshot);
  EXPECT_FALSE (target.useColor ());
}

// Test edge cases
TEST_F (ArrangerObjectColorTest, EdgeCases)
{
  // Invalid color
  QColor invalid_color;
  color_obj->setColor (invalid_color);
  EXPECT_FALSE (color_obj->useColor ());

  // Partially defined color
  QColor partial_color (128, 128, 128, 0); // Fully transparent
  color_obj->setColor (partial_color);
  EXPECT_TRUE (color_obj->useColor ());
  EXPECT_EQ (color_obj->color ().alpha (), 0);

  // Unset when no color is set
  EXPECT_NO_THROW (color_obj->unsetColor ());
}

} // namespace zrythm::structure::arrangement
