// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/curve.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::dsp;

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

TEST (CurveTest, FadeOperations)
{
  CurveOptions opts (0.5, CurveOptions::Algorithm::Exponent);
  const double epsilon = 0.0001;

  // Test fade in
  EXPECT_NEAR (opts.get_normalized_y_for_fade (0.0, true), 0.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y_for_fade (1.0, true), 1.0, epsilon);

  // Test fade out
  EXPECT_NEAR (opts.get_normalized_y_for_fade (0.0, false), 1.0, epsilon);
  EXPECT_NEAR (opts.get_normalized_y_for_fade (1.0, false), 0.0, epsilon);
}

TEST (CurveTest, Serialization)
{
  // Create curve options with specific values
  CurveOptions opts1 (0.75, CurveOptions::Algorithm::Vital);

  // Serialize to JSON
  auto json_str = opts1.serialize_to_json_string ();

  // Create new object and deserialize
  CurveOptions opts2;
  opts2.deserialize_from_json_string (json_str.c_str ());

  // Verify all fields match
  EXPECT_DOUBLE_EQ (opts1.curviness_, opts2.curviness_);
  EXPECT_EQ (opts1.algo_, opts2.algo_);
}
