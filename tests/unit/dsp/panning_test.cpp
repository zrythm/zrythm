// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/panning.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::dsp
{

class PanningTest : public ::testing::Test
{
protected:
  static constexpr float EPSILON = 0.0001f;

  const std::vector<float> test_positions_ = {
    0.0f, 0.1f, 0.25f, 0.33f, 0.5f, 0.67f, 0.75f, 0.9f, 1.0f
  };

  static float db_to_linear (float db) { return std::pow (10.0f, db / 20.0f); }
};

TEST_F (PanningTest, LinearAlgorithmBehavior)
{
  for (float pos : test_positions_)
    {
      auto [left, right] =
        calculate_panning (PanLaw::ZerodB, PanAlgorithm::Linear, pos);
      EXPECT_NEAR (left + right, 1.0f, EPSILON) << "Position: " << pos;
      EXPECT_NEAR (left, 1.0f - pos, EPSILON) << "Position: " << pos;
      EXPECT_NEAR (right, pos, EPSILON) << "Position: " << pos;
    }
}

TEST_F (PanningTest, SquareRootAlgorithmBehavior)
{
  for (float pos : test_positions_)
    {
      auto [left, right] =
        calculate_panning (PanLaw::ZerodB, PanAlgorithm::SquareRoot, pos);
      EXPECT_NEAR (left * left + right * right, 1.0f, EPSILON)
        << "Position: " << pos;
    }
}

TEST_F (PanningTest, SineLawBehavior)
{
  for (float pos : test_positions_)
    {
      auto [left, right] =
        calculate_panning (PanLaw::ZerodB, PanAlgorithm::SineLaw, pos);
      EXPECT_NEAR (left * left + right * right, 1.0f, EPSILON)
        << "Position: " << pos;
    }
}

TEST_F (PanningTest, PanLawCompensationValues)
{
  const float center = 0.5f;

  auto [left_3db, right_3db] =
    calculate_panning (PanLaw::Minus3dB, PanAlgorithm::Linear, center);
  auto [left_6db, right_6db] =
    calculate_panning (PanLaw::Minus6dB, PanAlgorithm::Linear, center);

  // At center position (0.5), each channel should get half of the compensated
  // gain
  const float expected_3db = db_to_linear (-3.0f) * 0.5f;
  const float expected_6db = db_to_linear (-6.0f) * 0.5f;

  EXPECT_NEAR (left_3db, expected_3db, EPSILON);
  EXPECT_NEAR (right_3db, expected_3db, EPSILON);
  EXPECT_NEAR (left_6db, expected_6db, EPSILON);
  EXPECT_NEAR (right_6db, expected_6db, EPSILON);
}

TEST_F (PanningTest, MonotonicityCheck)
{
  for (auto law : { PanLaw::ZerodB, PanLaw::Minus3dB, PanLaw::Minus6dB })
    {
      for (
        auto algo :
        { PanAlgorithm::Linear, PanAlgorithm::SquareRoot, PanAlgorithm::SineLaw })
        {
          float prev_left = 1.0f;
          float prev_right = 0.0f;

          for (float pos : test_positions_)
            {
              auto [left, right] = calculate_panning (law, algo, pos);
              EXPECT_LE (left, prev_left)
                << "Position: " << pos
                << " Algorithm: " << static_cast<int> (algo);
              EXPECT_GE (right, prev_right)
                << "Position: " << pos
                << " Algorithm: " << static_cast<int> (algo);
              prev_left = left;
              prev_right = right;
            }
        }
    }
}

TEST_F (PanningTest, EdgeCases)
{
  std::vector<float> edge_cases = {
    -1.0f,
    -0.1f,
    1.1f,
    2.0f,
    std::numeric_limits<float>::infinity (),
    -std::numeric_limits<float>::infinity (),
    // std::numeric_limits<float>::quiet_NaN ()
  };

  for (float pos : edge_cases)
    {
      auto [left, right] =
        calculate_panning (PanLaw::ZerodB, PanAlgorithm::Linear, pos);
      EXPECT_GE (left, 0.0f);
      EXPECT_LE (left, 1.0f);
      EXPECT_GE (right, 0.0f);
      EXPECT_LE (right, 1.0f);
    }
}

TEST_F (PanningTest, LinearBalanceControlBehavior)
{
  // Test center position
  auto [left_center, right_center] =
    calculate_balance_control (BalanceControlAlgorithm::Linear, 0.5f);
  EXPECT_NEAR (left_center, 1.0f, EPSILON);
  EXPECT_NEAR (right_center, 1.0f, EPSILON);

  // Test full left
  auto [left_full, right_full] =
    calculate_balance_control (BalanceControlAlgorithm::Linear, 0.0f);
  EXPECT_NEAR (left_full, 1.0f, EPSILON);
  EXPECT_NEAR (right_full, 0.0f, EPSILON);

  // Test full right
  auto [left_right, right_right] =
    calculate_balance_control (BalanceControlAlgorithm::Linear, 1.0f);
  EXPECT_NEAR (left_right, 0.0f, EPSILON);
  EXPECT_NEAR (right_right, 1.0f, EPSILON);

  // Test intermediate positions
  for (float pos : test_positions_)
    {
      auto [left, right] =
        calculate_balance_control (BalanceControlAlgorithm::Linear, pos);

      // Verify one channel always stays at unity gain
      if (pos < 0.5f)
        {
          EXPECT_NEAR (left, 1.0f, EPSILON) << "Position: " << pos;
          EXPECT_NEAR (right, pos * 2.0f, EPSILON) << "Position: " << pos;
        }
      else
        {
          EXPECT_NEAR (left, (1.0f - pos) * 2.0f, EPSILON)
            << "Position: " << pos;
          EXPECT_NEAR (right, 1.0f, EPSILON) << "Position: " << pos;
        }

      // Verify gains are in valid range
      EXPECT_GE (left, 0.0f) << "Position: " << pos;
      EXPECT_LE (left, 1.0f) << "Position: " << pos;
      EXPECT_GE (right, 0.0f) << "Position: " << pos;
      EXPECT_LE (right, 1.0f) << "Position: " << pos;
    }
}

TEST_F (PanningTest, BalanceControlEdgeCases)
{
  std::vector<float> edge_cases = {
    -1.0f,
    -0.1f,
    1.1f,
    2.0f,
    std::numeric_limits<float>::infinity (),
    -std::numeric_limits<float>::infinity (),
  };

  for (float pos : edge_cases)
    {
      auto [left, right] =
        calculate_balance_control (BalanceControlAlgorithm::Linear, pos);

      // Verify gains are clamped to valid range
      EXPECT_GE (left, 0.0f) << "Position: " << pos;
      EXPECT_LE (left, 1.0f) << "Position: " << pos;
      EXPECT_GE (right, 0.0f) << "Position: " << pos;
      EXPECT_LE (right, 1.0f) << "Position: " << pos;
    }
}

} // namespace zrythm::dsp
