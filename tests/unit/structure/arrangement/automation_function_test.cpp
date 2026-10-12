// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/automation_function.h"
#include "structure/arrangement/automation_point.h"

#include "helpers/mock_qobject.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{

class AutomationFunctionTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    parent = std::make_unique<MockQObject> ();
  }

  std::unique_ptr<AutomationPoint>
  make_point (double position_ticks, float value, double curviness)
  {
    auto point =
      std::make_unique<AutomationPoint> (*tempo_map_wrapper, parent.get ());
    point->position ()->set_ticks (
      dsp::ContentTick{ units::ticks (position_ticks) });
    point->setValue (value);
    point->curveOpts ()->setCurviness (curviness);
    return point;
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  std::unique_ptr<MockQObject>          parent;
};

TEST_F (AutomationFunctionTest, FlipVerticalInvertsValuesAndCurveDirection)
{
  auto                           p1 = make_point (100.0, 0.2f, 0.4);
  auto                           p2 = make_point (200.0, 0.9f, -0.3);
  std::vector<AutomationPoint *> points{ p1.get (), p2.get () };

  const auto result = AutomationFunction::FlipVertical::compute (points, {});

  EXPECT_FLOAT_EQ (result.new_states_[0].value_, 0.8f);
  EXPECT_FLOAT_EQ (result.new_states_[1].value_, 0.1f);
  EXPECT_DOUBLE_EQ (result.new_states_[0].curve_curviness_, -0.4);
  EXPECT_DOUBLE_EQ (result.new_states_[1].curve_curviness_, 0.3);
  // Positions and algorithms stay
  EXPECT_EQ (
    result.new_states_[0].position_ticks_,
    dsp::ContentTick{ units::ticks (100.0) });
  EXPECT_EQ (
    result.new_states_[0].curve_algorithm_, p1->curveOpts ()->algorithm ());
}

TEST_F (AutomationFunctionTest, FlipHorizontalMirrorsPositionsAcrossSpan)
{
  auto                           p1 = make_point (100.0, 0.2f, 0.4);
  auto                           p2 = make_point (300.0, 0.5f, 0.0);
  auto                           p3 = make_point (500.0, 0.8f, -0.6);
  std::vector<AutomationPoint *> points{ p1.get (), p2.get (), p3.get () };

  const auto result = AutomationFunction::FlipHorizontal::compute (points, {});

  // Points trade places from the outside in; the middle stays
  EXPECT_EQ (
    result.new_states_[0].position_ticks_,
    dsp::ContentTick{ units::ticks (500.0) });
  EXPECT_EQ (
    result.new_states_[1].position_ticks_,
    dsp::ContentTick{ units::ticks (300.0) });
  EXPECT_EQ (
    result.new_states_[2].position_ticks_,
    dsp::ContentTick{ units::ticks (100.0) });
  // Values stay
  EXPECT_FLOAT_EQ (result.new_states_[0].value_, 0.2f);
  EXPECT_FLOAT_EQ (result.new_states_[2].value_, 0.8f);
  // Curve direction reverses
  EXPECT_DOUBLE_EQ (result.new_states_[0].curve_curviness_, -0.4);
  EXPECT_DOUBLE_EQ (result.new_states_[2].curve_curviness_, 0.6);
}

TEST_F (AutomationFunctionTest, FlipHorizontalWithSinglePointKeepsPosition)
{
  auto                           p1 = make_point (100.0, 0.2f, 0.4);
  std::vector<AutomationPoint *> points{ p1.get () };

  const auto result = AutomationFunction::FlipHorizontal::compute (points, {});

  EXPECT_EQ (
    result.new_states_[0].position_ticks_,
    dsp::ContentTick{ units::ticks (100.0) });
  EXPECT_DOUBLE_EQ (result.new_states_[0].curve_curviness_, -0.4);
}

TEST_F (AutomationFunctionTest, FlattenReplacesCurveWithStep)
{
  auto                           p1 = make_point (100.0, 0.2f, -0.2);
  std::vector<AutomationPoint *> points{ p1.get () };

  const auto result = AutomationFunction::Flatten::compute (points, {});

  EXPECT_EQ (
    result.new_states_[0].curve_algorithm_, dsp::CurveOptions::Algorithm::Pulse);
  EXPECT_DOUBLE_EQ (result.new_states_[0].curve_curviness_, 1.0);
  EXPECT_FLOAT_EQ (result.new_states_[0].value_, 0.2f);
}

TEST_F (AutomationFunctionTest, ApplyWritesComputedStates)
{
  auto                           p1 = make_point (100.0, 0.2f, 0.4);
  std::vector<AutomationPoint *> points{ p1.get () };

  AutomationFunction::apply (
    points, AutomationFunction::Kind{ AutomationFunction::FlipVertical{} });

  EXPECT_FLOAT_EQ (p1->value (), 0.8f);
  EXPECT_DOUBLE_EQ (p1->curveOpts ()->curviness (), -0.4);
  EXPECT_EQ (
    p1->position ()->asTick (), dsp::ContentTick{ units::ticks (100.0) });
}

TEST_F (AutomationFunctionTest, KindFromIdFindsEveryKind)
{
  for (
    const auto &[id, expected_label] : {
      std::pair{
                AutomationFunction::FlipHorizontal::kId,
                utils::Utf8String (u8"Flip Horizontal")                                  },
      { AutomationFunction::FlipVertical::kId,
                utils::Utf8String (u8"Flip Vertical")                                    },
      { AutomationFunction::Flatten::kId,       utils::Utf8String (u8"Flatten") }
  })
    {
      const auto kind = AutomationFunction::kind_from_id (id);
      ASSERT_TRUE (kind) << id;
      EXPECT_EQ (
        AutomationFunction::label (*kind).view (), expected_label.view ());
    }
}

TEST_F (AutomationFunctionTest, KindFromIdRejectsUnknownId)
{
  EXPECT_FALSE (AutomationFunction::kind_from_id ("no-such-function"));
}

} // namespace zrythm::structure::arrangement
