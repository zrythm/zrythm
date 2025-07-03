// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/curve.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::dsp
{

TEST (CurveTest, BasicConstruction)
{
  CurveOptions opts (0.5, CurveOptions::Algorithm::Exponent);
  EXPECT_DOUBLE_EQ (opts.curviness_, 0.5);
  EXPECT_EQ (opts.algo_, CurveOptions::Algorithm::Exponent);
}

TEST (CurveTest, ExponentCurve)
{
  CurveOptions opts (0.0, CurveOptions::Algorithm::Exponent);
  const double epsilon = 0.0001;

  // Test linear case (curviness = 0)
  EXPECT_NEAR (opts.get_normalized_y (0.0, false), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (0.5, false), 0.5, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (1.0, false), 1.0, epsilon);

  // Test curved case
  opts.curviness_ = 0.5;
  EXPECT_NEAR (opts.get_normalized_y (0.0, false), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (0.5, false), 0.69496, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (1.0, false), 1.0, epsilon);
}

TEST (CurveTest, SuperEllipseCurve)
{
  CurveOptions opts (0.0, CurveOptions::Algorithm::SuperEllipse);
  const double epsilon = 0.0001;

  // Test linear case
  EXPECT_NEAR (opts.get_normalized_y (0.0, false), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (0.5, false), 0.5, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (1.0, false), 1.0, epsilon);

  // Test curved case
  opts.curviness_ = 0.7;
  EXPECT_NEAR (opts.get_normalized_y (0.0, false), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (0.5, false), 0.9593, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (1.0, false), 1.0, epsilon);
}

TEST (CurveTest, VitalCurve)
{
  CurveOptions opts (0.0, CurveOptions::Algorithm::Vital);
  const double epsilon = 0.0001;

  // Test linear case
  EXPECT_NEAR (opts.get_normalized_y (0.0, false), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (0.5, false), 0.5, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (1.0, false), 1.0, epsilon);

  // Test curved case
  opts.curviness_ = 0.5;
  EXPECT_NEAR (opts.get_normalized_y (0.0, false), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (0.5, false), 0.9241, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (1.0, false), 1.0, epsilon);
}

TEST (CurveTest, PulseCurve)
{
  CurveOptions opts (0.0, CurveOptions::Algorithm::Pulse);
  const double epsilon = 0.0001;

  // Test default case
  EXPECT_NEAR (opts.get_normalized_y (0.0, false), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (1.0, false), 1.0, epsilon);

  // Test with positive curviness
  opts.curviness_ = 0.5;
  EXPECT_NEAR (opts.get_normalized_y (0.0, false), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (0.5, false), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (1.0, false), 1.0, epsilon);
}

TEST (CurveTest, LogarithmicCurve)
{
  CurveOptions opts (0.0, CurveOptions::Algorithm::Logarithmic);
  const double epsilon = 0.0001;

  // Test linear case
  EXPECT_NEAR (opts.get_normalized_y (0.0, false), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (0.5, false), 0.511909664, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (1.0, false), 1.0, epsilon);

  // Test curved case
  opts.curviness_ = -0.5;
  EXPECT_NEAR (opts.get_normalized_y (0.0, false), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (0.5, false), 1 - 0.893168449, epsilon);
  EXPECT_NEAR (opts.get_normalized_y (1.0, false), 1.0, epsilon);
}

TEST (CurveTest, Serialization)
{
  // Create curve options with specific values
  CurveOptions opts1 (0.75, CurveOptions::Algorithm::Vital);

  // Serialize to JSON
  nlohmann::json j = opts1;

  // Create new object and deserialize
  CurveOptions opts2;
  from_json (j, opts2);

  // Verify all fields match
  EXPECT_DOUBLE_EQ (opts1.curviness_, opts2.curviness_);
  EXPECT_EQ (opts1.algo_, opts2.algo_);
}

TEST (CurveTest, CurveOptionsQmlAdapter)
{
  CurveOptions           options (0.5, CurveOptions::Algorithm::Exponent);
  CurveOptionsQmlAdapter adapter (options);

  // Test initial state
  EXPECT_DOUBLE_EQ (adapter.curviness (), 0.5);
  EXPECT_EQ (
    adapter.algorithm (), ENUM_VALUE_TO_INT (CurveOptions::Algorithm::Exponent));

  // Test setting curviness
  adapter.setCurviness (0.7);
  EXPECT_DOUBLE_EQ (options.curviness_, 0.7);
  EXPECT_DOUBLE_EQ (adapter.curviness (), 0.7);

  // Test setting algorithm
  adapter.setAlgorithm (
    ENUM_VALUE_TO_INT (CurveOptions::Algorithm::SuperEllipse));
  EXPECT_EQ (options.algo_, CurveOptions::Algorithm::SuperEllipse);
  EXPECT_EQ (
    adapter.algorithm (),
    ENUM_VALUE_TO_INT (CurveOptions::Algorithm::SuperEllipse));
}

TEST (CurveTest, CurveOptionsQmlAdapterSignals)
{
  MockQObject            mockQObject;
  CurveOptions           options (0.5, CurveOptions::Algorithm::Exponent);
  CurveOptionsQmlAdapter adapter (options, &mockQObject);

  // Setup signal watchers
  testing::MockFunction<void (double)> mockCurvinessChanged;
  testing::MockFunction<void (int)>    mockAlgorithmChanged;

  QObject::connect (
    &adapter, &CurveOptionsQmlAdapter::curvinessChanged, &mockQObject,
    [&] (double curviness) { mockCurvinessChanged.Call (curviness); });

  QObject::connect (
    &adapter, &CurveOptionsQmlAdapter::algorithmChanged, &mockQObject,
    [&] (int algo) { mockAlgorithmChanged.Call (algo); });

  // Test curvinessChanged signal
  EXPECT_CALL (mockCurvinessChanged, Call (0.75)).Times (1);
  adapter.setCurviness (0.75);

  // Test no signal when same value
  EXPECT_CALL (mockCurvinessChanged, Call (0.75)).Times (0);
  adapter.setCurviness (0.75);

  // Test algorithmChanged signal
  int newAlgo = ENUM_VALUE_TO_INT (CurveOptions::Algorithm::Vital);
  EXPECT_CALL (mockAlgorithmChanged, Call (newAlgo)).Times (1);
  adapter.setAlgorithm (newAlgo);

  // Test no signal when same value
  EXPECT_CALL (mockAlgorithmChanged, Call (newAlgo)).Times (0);
  adapter.setAlgorithm (newAlgo);
}

TEST (CurveTest, CurveOptionsQmlAdapterEdgeCases)
{
  CurveOptions           options (0.0, CurveOptions::Algorithm::Exponent);
  CurveOptionsQmlAdapter adapter (options);

  // Test invalid curviness values
  adapter.setCurviness (1.5);
  EXPECT_DOUBLE_EQ (adapter.curviness (), 1.0);

  adapter.setCurviness (-1.5);
  EXPECT_DOUBLE_EQ (adapter.curviness (), -1.0);

  // Test invalid algorithm values
  adapter.setAlgorithm (100);
  EXPECT_EQ (adapter.algorithm (), ENUM_COUNT (CurveOptions::Algorithm) - 1);

  adapter.setAlgorithm (-1);
  EXPECT_EQ (
    adapter.algorithm (), ENUM_VALUE_TO_INT (CurveOptions::Algorithm::Exponent));
}
}
