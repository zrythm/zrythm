// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "structure/arrangement/editor_settings.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{

class EditorSettingsTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create a QCoreApplication for signal testing
    app = std::make_unique<test_helpers::ScopedQCoreApplication> ();

    // Create EditorSettings object
    settings = std::make_unique<EditorSettings> ();
  }

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app;
  std::unique_ptr<EditorSettings>                       settings;
};

// Test initial state
TEST_F (EditorSettingsTest, InitialState)
{
  EXPECT_DOUBLE_EQ (settings->getX (), 0.0);
  EXPECT_DOUBLE_EQ (settings->getY (), 0.0);
  EXPECT_DOUBLE_EQ (settings->getHorizontalZoomLevel (), 1.0);
}

// Test X property getter and setter
TEST_F (EditorSettingsTest, XProperty)
{
  // Test setting positive value
  QSignalSpy xChangedSpy (settings.get (), &EditorSettings::xChanged);

  settings->setX (100.5);
  EXPECT_DOUBLE_EQ (settings->getX (), 100.5);
  EXPECT_EQ (xChangedSpy.count (), 1);
  EXPECT_DOUBLE_EQ (xChangedSpy.at (0).at (0).toDouble (), 100.5);

  // Test setting negative value (should be clamped to 0)
  xChangedSpy.clear ();
  settings->setX (-50.0);
  EXPECT_DOUBLE_EQ (settings->getX (), 0.0);
  EXPECT_EQ (xChangedSpy.count (), 1);
  EXPECT_DOUBLE_EQ (xChangedSpy.at (0).at (0).toDouble (), 0.0);

  // Test setting zero (already default, so no signal should be emitted)
  xChangedSpy.clear ();
  settings->setX (0.0);
  EXPECT_DOUBLE_EQ (settings->getX (), 0.0);
  EXPECT_EQ (xChangedSpy.count (), 0); // No signal for same value as current

  // Test setting same value (should not emit signal)
  xChangedSpy.clear ();
  settings->setX (0.0);
  EXPECT_DOUBLE_EQ (settings->getX (), 0.0);
  EXPECT_EQ (xChangedSpy.count (), 0); // No signal for same value
}

// Test Y property getter and setter
TEST_F (EditorSettingsTest, YProperty)
{
  // Test setting positive value
  QSignalSpy yChangedSpy (settings.get (), &EditorSettings::yChanged);

  settings->setY (200.75);
  EXPECT_DOUBLE_EQ (settings->getY (), 200.75);
  EXPECT_EQ (yChangedSpy.count (), 1);
  EXPECT_DOUBLE_EQ (yChangedSpy.at (0).at (0).toDouble (), 200.75);

  // Test setting negative value (should be clamped to 0)
  yChangedSpy.clear ();
  settings->setY (-100.0);
  EXPECT_DOUBLE_EQ (settings->getY (), 0.0);
  EXPECT_EQ (yChangedSpy.count (), 1);
  EXPECT_DOUBLE_EQ (yChangedSpy.at (0).at (0).toDouble (), 0.0);

  // Test setting zero (already default, so no signal should be emitted)
  yChangedSpy.clear ();
  settings->setY (0.0);
  EXPECT_DOUBLE_EQ (settings->getY (), 0.0);
  EXPECT_EQ (yChangedSpy.count (), 0); // No signal for same value as current

  // Test setting same value (should not emit signal)
  yChangedSpy.clear ();
  settings->setY (0.0);
  EXPECT_DOUBLE_EQ (settings->getY (), 0.0);
  EXPECT_EQ (yChangedSpy.count (), 0); // No signal for same value
}

// Test HorizontalZoomLevel property getter and setter
TEST_F (EditorSettingsTest, HorizontalZoomLevelProperty)
{
  // Test setting various zoom levels
  QSignalSpy hzoomChangedSpy (
    settings.get (), &EditorSettings::horizontalZoomLevelChanged);

  settings->setHorizontalZoomLevel (2.5);
  EXPECT_DOUBLE_EQ (settings->getHorizontalZoomLevel (), 2.5);
  EXPECT_EQ (hzoomChangedSpy.count (), 1);
  EXPECT_DOUBLE_EQ (hzoomChangedSpy.at (0).at (0).toDouble (), 2.5);

  // Test setting small zoom level
  hzoomChangedSpy.clear ();
  settings->setHorizontalZoomLevel (0.1);
  EXPECT_DOUBLE_EQ (settings->getHorizontalZoomLevel (), 0.1);
  EXPECT_EQ (hzoomChangedSpy.count (), 1);
  EXPECT_DOUBLE_EQ (hzoomChangedSpy.at (0).at (0).toDouble (), 0.1);

  // Test setting large zoom level
  hzoomChangedSpy.clear ();
  settings->setHorizontalZoomLevel (10.0);
  EXPECT_DOUBLE_EQ (settings->getHorizontalZoomLevel (), 10.0);
  EXPECT_EQ (hzoomChangedSpy.count (), 1);
  EXPECT_DOUBLE_EQ (hzoomChangedSpy.at (0).at (0).toDouble (), 10.0);

  // Test setting same value (should not emit signal)
  hzoomChangedSpy.clear ();
  settings->setHorizontalZoomLevel (10.0);
  EXPECT_DOUBLE_EQ (settings->getHorizontalZoomLevel (), 10.0);
  EXPECT_EQ (hzoomChangedSpy.count (), 0); // No signal for same value
}

// Test clamp_scroll_start_x method
TEST_F (EditorSettingsTest, ClampScrollStartX)
{
  // Test positive values (should return as-is)
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_x (100.0), 100.0);
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_x (0.1), 0.1);
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_x (1e6), 1e6);

  // Test zero (should return zero)
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_x (0.0), 0.0);

  // Test negative values (should be clamped to 0)
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_x (-1.0), 0.0);
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_x (-100.5), 0.0);
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_x (-1e-10), 0.0);
}

// Test clamp_scroll_start_y method
TEST_F (EditorSettingsTest, ClampScrollStartY)
{
  // Test positive values (should return as-is)
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_y (200.0), 200.0);
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_y (0.5), 0.5);
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_y (1e6), 1e6);

  // Test zero (should return zero)
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_y (0.0), 0.0);

  // Test negative values (should be clamped to 0)
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_y (-1.0), 0.0);
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_y (-50.25), 0.0);
  EXPECT_DOUBLE_EQ (settings->clamp_scroll_start_y (-1e-10), 0.0);
}

// Test append_scroll method
TEST_F (EditorSettingsTest, AppendScroll)
{
  // Store initial values
  const double initial_x = settings->getX ();
  const double initial_y = settings->getY ();

  // Test append_scroll with validate=false (currently no-op)
  settings->append_scroll (10.0, 20.0, false);
  // Since append_scroll is currently a no-op (TODO), values should remain
  // unchanged
  EXPECT_DOUBLE_EQ (settings->getX (), initial_x);
  EXPECT_DOUBLE_EQ (settings->getY (), initial_y);

  // Test append_scroll with validate=true (currently no-op)
  settings->append_scroll (-5.0, -15.0, true);
  // Since append_scroll is currently a no-op (TODO), values should remain
  // unchanged
  EXPECT_DOUBLE_EQ (settings->getX (), initial_x);
  EXPECT_DOUBLE_EQ (settings->getY (), initial_y);
}

// Test serialization/deserialization
TEST_F (EditorSettingsTest, Serialization)
{
  // Set initial state
  settings->setX (150.25);
  settings->setY (75.5);
  settings->setHorizontalZoomLevel (3.2);

  // Serialize to JSON
  nlohmann::json j;
  to_json (j, *settings);

  // Verify JSON content
  EXPECT_TRUE (j.contains ("scrollStartX"));
  EXPECT_TRUE (j.contains ("scrollStartY"));
  EXPECT_TRUE (j.contains ("hZoomLevel"));
  EXPECT_DOUBLE_EQ (j["scrollStartX"].get<double> (), 150.25);
  EXPECT_DOUBLE_EQ (j["scrollStartY"].get<double> (), 75.5);
  EXPECT_DOUBLE_EQ (j["hZoomLevel"].get<double> (), 3.2);

  // Create new object and deserialize
  auto new_settings = std::make_unique<EditorSettings> ();
  from_json (j, *new_settings);

  // Verify deserialized state
  EXPECT_DOUBLE_EQ (new_settings->getX (), 150.25);
  EXPECT_DOUBLE_EQ (new_settings->getY (), 75.5);
  EXPECT_DOUBLE_EQ (new_settings->getHorizontalZoomLevel (), 3.2);
}

// Test serialization with default values
TEST_F (EditorSettingsTest, SerializationWithDefaults)
{
  // Create object with default values
  auto default_settings = std::make_unique<EditorSettings> ();

  // Serialize to JSON
  nlohmann::json j;
  to_json (j, *default_settings);

  // Verify JSON content contains default values
  EXPECT_DOUBLE_EQ (j["scrollStartX"].get<double> (), 0.0);
  EXPECT_DOUBLE_EQ (j["scrollStartY"].get<double> (), 0.0);
  EXPECT_DOUBLE_EQ (j["hZoomLevel"].get<double> (), 1.0);

  // Create new object and deserialize
  auto new_settings = std::make_unique<EditorSettings> ();
  from_json (j, *new_settings);

  // Verify deserialized state matches defaults
  EXPECT_DOUBLE_EQ (new_settings->getX (), 0.0);
  EXPECT_DOUBLE_EQ (new_settings->getY (), 0.0);
  EXPECT_DOUBLE_EQ (new_settings->getHorizontalZoomLevel (), 1.0);
}

// Test copying using init_from
TEST_F (EditorSettingsTest, Copying)
{
  // Set source object state
  settings->setX (300.75);
  settings->setY (150.25);
  settings->setHorizontalZoomLevel (5.5);

  // Create target object
  auto target_settings = std::make_unique<EditorSettings> ();

  // Copy using init_from
  init_from (*target_settings, *settings, utils::ObjectCloneType::Snapshot);

  // Verify copied state
  EXPECT_DOUBLE_EQ (target_settings->getX (), 300.75);
  EXPECT_DOUBLE_EQ (target_settings->getY (), 150.25);
  EXPECT_DOUBLE_EQ (target_settings->getHorizontalZoomLevel (), 5.5);
}

// Test edge cases and floating point precision
TEST_F (EditorSettingsTest, EdgeCases)
{
  // Test very small values
  settings->setX (1e-10);
  EXPECT_DOUBLE_EQ (settings->getX (), 1e-10);

  settings->setY (1e-15);
  EXPECT_DOUBLE_EQ (settings->getY (), 1e-15);

  settings->setHorizontalZoomLevel (1e-20);
  EXPECT_DOUBLE_EQ (settings->getHorizontalZoomLevel (), 1e-20);

  // Test very large values
  settings->setX (1e10);
  EXPECT_DOUBLE_EQ (settings->getX (), 1e10);

  settings->setY (1e15);
  EXPECT_DOUBLE_EQ (settings->getY (), 1e15);

  settings->setHorizontalZoomLevel (1e20);
  EXPECT_DOUBLE_EQ (settings->getHorizontalZoomLevel (), 1e20);

  // Test infinity and NaN handling (if applicable)
  // Note: Qt properties may handle these differently
  settings->setX (std::numeric_limits<double>::infinity ());
  // The behavior depends on the implementation - we just test it doesn't crash
  EXPECT_TRUE (std::isinf (settings->getX ()) || std::isnan (settings->getX ()));
}

// Test multiple signal emissions
TEST_F (EditorSettingsTest, MultipleSignalEmissions)
{
  QSignalSpy xChangedSpy (settings.get (), &EditorSettings::xChanged);
  QSignalSpy yChangedSpy (settings.get (), &EditorSettings::yChanged);
  QSignalSpy hzoomChangedSpy (
    settings.get (), &EditorSettings::horizontalZoomLevelChanged);

  // Change multiple properties
  settings->setX (100.0);
  settings->setY (200.0);
  settings->setHorizontalZoomLevel (2.5);

  // Verify all signals were emitted
  EXPECT_EQ (xChangedSpy.count (), 1);
  EXPECT_EQ (yChangedSpy.count (), 1);
  EXPECT_EQ (hzoomChangedSpy.count (), 1);

  // Verify signal values
  EXPECT_DOUBLE_EQ (xChangedSpy.at (0).at (0).toDouble (), 100.0);
  EXPECT_DOUBLE_EQ (yChangedSpy.at (0).at (0).toDouble (), 200.0);
  EXPECT_DOUBLE_EQ (hzoomChangedSpy.at (0).at (0).toDouble (), 2.5);
}

// Test object parenting
TEST_F (EditorSettingsTest, ObjectParenting)
{
  // Create EditorSettings with a parent
  QObject parent;
  auto    child_settings = std::make_unique<EditorSettings> (&parent);

  // Verify parent relationship
  EXPECT_EQ (child_settings->parent (), &parent);

  // Test that properties still work with parent
  child_settings->setX (50.0);
  EXPECT_DOUBLE_EQ (child_settings->getX (), 50.0);
}

} // namespace zrythm::structure::arrangement
