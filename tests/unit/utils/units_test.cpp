// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/units.h"

using namespace zrythm::units;

TEST (UnitsTest, TickTypes)
{
  // Test that tick_t and precise_tick_t are defined correctly
  tick_t         int_tick = ticks (100);
  precise_tick_t double_tick = ticks (100.5);

  // Test that we can create quantities with the correct types
  EXPECT_EQ (int_tick, ticks (100));
  EXPECT_EQ (double_tick, ticks (100.5));
}

TEST (UnitsTest, PPQConstant)
{
  // Test that PPQ is defined correctly
  EXPECT_EQ (PPQ, ticks (960));
}

TEST (UnitsTest, QuarterNoteToTicks)
{
  // Test that quarter note equals PPQ ticks
  auto quarter_in_ticks = quarter_notes (1);
  EXPECT_EQ (quarter_in_ticks.in (ticks), 960);
}

TEST (UnitsTest, TickQuantityOperations)
{
  // Test arithmetic operations with tick quantities
  tick_t tick1 = ticks (100);
  tick_t tick2 = ticks (50);
  tick_t tick3 = ticks (25);

  // Addition
  auto sum = tick1 + tick2;
  EXPECT_EQ (sum, ticks (150));

  // Subtraction
  auto diff = tick1 - tick2;
  EXPECT_EQ (diff, ticks (50));

  // Multiplication by scalar
  auto multiplied = tick1 * 2;
  EXPECT_EQ (multiplied, ticks (200));

  // Division by scalar
  auto divided = tick1 / 2;
  EXPECT_EQ (divided, ticks (50));

  // Compound operations
  auto complex = (tick1 + tick2) / 3 + tick3;
  EXPECT_EQ (complex, ticks (75));
}

TEST (UnitsTest, TickTypeConversions)
{
  // Test conversion from tick_t to precise_tick_t (allowed: int64_t -> double)
  tick_t int_tick = ticks (100);

  // Lambda that explicitly expects precise_tick_t
  auto precise_lambda = [] (precise_tick_t precise) -> double {
    return precise.in (ticks);
  };

  // Should be able to pass tick_t to function expecting precise_tick_t
  // (implicit conversion from int64_t to double is value-preserving)
  double result = precise_lambda (int_tick);
  EXPECT_DOUBLE_EQ (result, 100.0);

  // Test explicit conversion using coerce_as for value-truncating conversions
  precise_tick_t double_tick = ticks (100.5);

  // Convert precise_tick_t to tick_t using explicit cast (value-truncating)
  tick_t truncated_tick =
    double_tick.as<int64_t> (ticks, ignore (au::TRUNCATION_RISK));
  EXPECT_EQ (truncated_tick.in (ticks), 100);

  // Test with exact value that should not truncate
  precise_tick_t exact_tick = ticks (200.0);
  tick_t         exact_converted =
    exact_tick.as<int64_t> (ticks, ignore (au::TRUNCATION_RISK));
  EXPECT_EQ (exact_converted.in (ticks), 200);
}

TEST (UnitsTest, MixedTypeArithmetic)
{
  // Test arithmetic operations between tick_t and precise_tick_t
  tick_t         int_tick = ticks (100);
  precise_tick_t double_tick = ticks (50.5);

  // Addition: precise_tick_t + tick_t should yield precise_tick_t
  auto sum1 = double_tick + int_tick;
  static_assert (std::is_same_v<decltype (sum1), precise_tick_t>);
  EXPECT_DOUBLE_EQ (sum1.in (ticks), 150.5);

  // Addition: tick_t + precise_tick_t should yield precise_tick_t
  auto sum2 = int_tick + double_tick;
  static_assert (std::is_same_v<decltype (sum2), precise_tick_t>);
  EXPECT_DOUBLE_EQ (sum2.in (ticks), 150.5);

  // Subtraction: precise_tick_t - tick_t should yield precise_tick_t
  auto diff1 = double_tick - int_tick;
  static_assert (std::is_same_v<decltype (diff1), precise_tick_t>);
  EXPECT_DOUBLE_EQ (diff1.in (ticks), -49.5);

  // Subtraction: tick_t - precise_tick_t should yield precise_tick_t
  auto diff2 = int_tick - double_tick;
  static_assert (std::is_same_v<decltype (diff2), precise_tick_t>);
  EXPECT_DOUBLE_EQ (diff2.in (ticks), 49.5);

  // Multiplication by scalar: int_tick * double yields precise_tick_t
  auto multiplied_int = int_tick * 2.5;
  static_assert (std::is_same_v<decltype (multiplied_int), precise_tick_t>);
  EXPECT_DOUBLE_EQ (multiplied_int.in (ticks), 250.0);

  // Multiplication by scalar: double_tick * int yields precise_tick_t
  auto multiplied_double = double_tick * 2;
  static_assert (std::is_same_v<decltype (multiplied_double), precise_tick_t>);
  EXPECT_DOUBLE_EQ (multiplied_double.in (ticks), 101.0);
}

TEST (UnitsTest, SampleRateOperations)
{
  // Test sample rate operations
  auto sample_rate_1 = sample_rate (44100.0);

  // Test that sample rate can be used in calculations with time
  auto duration = seconds (1.0);
  auto total_samples = sample_rate_1 * duration;

  // total_samples should be in samples
  EXPECT_DOUBLE_EQ (total_samples.in (samples), 44100.0);

  // Test division of samples by time to get sample rate
  auto samples_count = samples (88200);
  auto calculated_rate = samples_count / seconds (2.0);
  EXPECT_DOUBLE_EQ (calculated_rate.in (sample_rate), 44100.0);
}
