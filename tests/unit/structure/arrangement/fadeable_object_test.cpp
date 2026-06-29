// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/position.h"
#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/fadeable_object.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
class ArrangerObjectFadeRangeTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    parent = std::make_unique<MockQObject> ();
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (tempo_map);
    start_position = std::make_unique<dsp::Position> ();
    range = std::make_unique<ArrangerObjectFadeRange> (
      *start_position, *tempo_map_wrapper, parent.get ());
  }

  dsp::TempoMap                         tempo_map{ units::sample_rate (44100) };
  std::unique_ptr<MockQObject>          parent;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  std::unique_ptr<dsp::Position>        start_position;
  std::unique_ptr<ArrangerObjectFadeRange> range;
};

// Test initial state
TEST_F (ArrangerObjectFadeRangeTest, InitialState)
{
  EXPECT_EQ (range->startOffset ()->ticks (), 0);
  EXPECT_EQ (range->endOffset ()->ticks (), 0);

  // Test curve options defaults
  EXPECT_DOUBLE_EQ (range->fadeInCurveOpts ()->curviness (), 0.0);
  EXPECT_EQ (
    range->fadeInCurveOpts ()->algorithm (),
    dsp::CurveOptions::Algorithm::Exponent);
  EXPECT_DOUBLE_EQ (range->fadeOutCurveOpts ()->curviness (), 0.0);
  EXPECT_EQ (
    range->fadeOutCurveOpts ()->algorithm (),
    dsp::CurveOptions::Algorithm::Exponent);
}

// Test setting offsets
TEST_F (ArrangerObjectFadeRangeTest, SetOffsets)
{
  range->startOffset ()->setTicks (1000);
  range->endOffset ()->setTicks (2000);

  EXPECT_EQ (range->startOffset ()->ticks (), 1000);
  EXPECT_EQ (range->endOffset ()->ticks (), 2000);
}

// Test non-negative constraint
TEST_F (ArrangerObjectFadeRangeTest, NonNegativeConstraint)
{
  // Test that negative values are clamped to 0
  range->startOffset ()->setTicks (-100);
  EXPECT_EQ (range->startOffset ()->ticks (), 0);

  range->endOffset ()->setTicks (-200);
  EXPECT_EQ (range->endOffset ()->ticks (), 0);

  // Test that positive values work normally
  range->startOffset ()->setTicks (500);
  EXPECT_EQ (range->startOffset ()->ticks (), 500);

  range->endOffset ()->setTicks (1000);
  EXPECT_EQ (range->endOffset ()->ticks (), 1000);

  // Test with different units
  range->startOffset ()->setTicks (-1.0);
  EXPECT_EQ (range->startOffset ()->ticks (), 0);

  range->endOffset ()->setTicks (-480.0);
  EXPECT_EQ (range->endOffset ()->ticks (), 0);
}

// Test curve options
TEST_F (ArrangerObjectFadeRangeTest, CurveOptions)
{
  // Set curve options
  range->fadeInCurveOpts ()->setCurviness (0.7);
  range->fadeInCurveOpts ()->setAlgorithm (
    dsp::CurveOptions::Algorithm::Exponent);

  range->fadeOutCurveOpts ()->setCurviness (0.3);
  range->fadeOutCurveOpts ()->setAlgorithm (
    dsp::CurveOptions::Algorithm::Logarithmic);

  // Verify
  EXPECT_DOUBLE_EQ (range->fadeInCurveOpts ()->curviness (), 0.7);
  EXPECT_EQ (
    range->fadeInCurveOpts ()->algorithm (),
    dsp::CurveOptions::Algorithm::Exponent);
  EXPECT_DOUBLE_EQ (range->fadeOutCurveOpts ()->curviness (), 0.3);
  EXPECT_EQ (
    range->fadeOutCurveOpts ()->algorithm (),
    dsp::CurveOptions::Algorithm::Logarithmic);
}

TEST_F (ArrangerObjectFadeRangeTest, NormalizedYForFade)
{
  // Test fade in
  EXPECT_DOUBLE_EQ (range->get_normalized_y_for_fade (0.0, true), 0.0);
  EXPECT_DOUBLE_EQ (range->get_normalized_y_for_fade (1.0, true), 1.0);

  // Test fade out
  EXPECT_DOUBLE_EQ (range->get_normalized_y_for_fade (0.0, false), 1.0);
  EXPECT_DOUBLE_EQ (range->get_normalized_y_for_fade (1.0, false), 0.0);
}

// Test curve options signals
TEST_F (ArrangerObjectFadeRangeTest, CurveOptionsSignals)
{
  // Setup signal watchers
  testing::MockFunction<void (double)> mockFadeInCurvinessChanged;
  testing::MockFunction<void (dsp::CurveOptions::Algorithm)>
                                       mockFadeInAlgorithmChanged;
  testing::MockFunction<void (double)> mockFadeOutCurvinessChanged;
  testing::MockFunction<void (dsp::CurveOptions::Algorithm)>
    mockFadeOutAlgorithmChanged;

  QObject::connect (
    range->fadeInCurveOpts (), &dsp::CurveOptionsQmlAdapter::curvinessChanged,
    parent.get (), mockFadeInCurvinessChanged.AsStdFunction ());
  QObject::connect (
    range->fadeInCurveOpts (), &dsp::CurveOptionsQmlAdapter::algorithmChanged,
    parent.get (), mockFadeInAlgorithmChanged.AsStdFunction ());
  QObject::connect (
    range->fadeOutCurveOpts (), &dsp::CurveOptionsQmlAdapter::curvinessChanged,
    parent.get (), mockFadeOutCurvinessChanged.AsStdFunction ());
  QObject::connect (
    range->fadeOutCurveOpts (), &dsp::CurveOptionsQmlAdapter::algorithmChanged,
    parent.get (), mockFadeOutAlgorithmChanged.AsStdFunction ());

  // Test fade in signals
  EXPECT_CALL (mockFadeInCurvinessChanged, Call (0.8)).Times (1);
  EXPECT_CALL (
    mockFadeInAlgorithmChanged, Call (dsp::CurveOptions::Algorithm::Vital))
    .Times (1);
  range->fadeInCurveOpts ()->setCurviness (0.8);
  range->fadeInCurveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Vital);

  // Test fade out signals
  EXPECT_CALL (mockFadeOutCurvinessChanged, Call (0.4)).Times (1);
  EXPECT_CALL (
    mockFadeOutAlgorithmChanged,
    Call (dsp::CurveOptions::Algorithm::SuperEllipse))
    .Times (1);
  range->fadeOutCurveOpts ()->setCurviness (0.4);
  range->fadeOutCurveOpts ()->setAlgorithm (
    dsp::CurveOptions::Algorithm::SuperEllipse);
}

// Test serialization/deserialization
TEST_F (ArrangerObjectFadeRangeTest, Serialization)
{
  // Set initial state
  range->startOffset ()->setTicks (1000);
  range->endOffset ()->setTicks (2000);

  // Set curve options
  range->fadeInCurveOpts ()->setCurviness (0.7);
  range->fadeInCurveOpts ()->setAlgorithm (
    dsp::CurveOptions::Algorithm::Exponent);
  range->fadeOutCurveOpts ()->setCurviness (0.3);
  range->fadeOutCurveOpts ()->setAlgorithm (
    dsp::CurveOptions::Algorithm::Logarithmic);

  // Serialize
  nlohmann::json j;
  to_json (j, *range);

  // Create new range
  dsp::Position new_start_pos;
  auto          new_range = std::make_unique<ArrangerObjectFadeRange> (
    new_start_pos, *tempo_map_wrapper, parent.get ());
  from_json (j, *new_range);

  // Verify state
  EXPECT_EQ (new_range->startOffset ()->ticks (), 1000);
  EXPECT_EQ (new_range->endOffset ()->ticks (), 2000);
  EXPECT_DOUBLE_EQ (new_range->fadeInCurveOpts ()->curviness (), 0.7);
  EXPECT_EQ (
    new_range->fadeInCurveOpts ()->algorithm (),
    dsp::CurveOptions::Algorithm::Exponent);
  EXPECT_DOUBLE_EQ (new_range->fadeOutCurveOpts ()->curviness (), 0.3);
  EXPECT_EQ (
    new_range->fadeOutCurveOpts ()->algorithm (),
    dsp::CurveOptions::Algorithm::Logarithmic);
}

} // namespace zrythm::structure::arrangement
