// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/units.h"

#include <mp-units/systems/isq.h>

using namespace zrythm::units;

TEST (UnitsTest, TickTypes)
{
  // Test that tick_t and precise_tick_t are defined correctly
  tick_t         int_tick = 100 * tick;
  precise_tick_t double_tick = 100.5 * tick;

  // Test that we can create quantities with the correct types
  EXPECT_EQ (int_tick, 100 * tick);
  EXPECT_EQ (double_tick, 100.5 * tick);
}

TEST (UnitsTest, PPQConstant)
{
  // Test that PPQ is defined correctly
  EXPECT_EQ (PPQ, 960 * tick);
}

TEST (UnitsTest, QuarterNoteToTicks)
{
  // Test that quarter note equals PPQ ticks
  auto quarter_in_ticks = 1 * quarter_note;
  EXPECT_EQ (quarter_in_ticks, PPQ);
}

TEST (UnitsTest, TickQuantityOperations)
{
  // Test arithmetic operations with tick quantities
  tick_t tick1 = 100 * tick;
  tick_t tick2 = 50 * tick;
  tick_t tick3 = 25 * tick;

  // Addition
  auto sum = tick1 + tick2;
  EXPECT_EQ (sum, 150 * tick);

  // Subtraction
  auto diff = tick1 - tick2;
  EXPECT_EQ (diff, 50 * tick);

  // Multiplication by scalar
  auto multiplied = tick1 * 2;
  EXPECT_EQ (multiplied, 200 * tick);

  // Division by scalar
  auto divided = tick1 / 2;
  EXPECT_EQ (divided, 50 * tick);

  // Compound operations
  auto complex = (tick1 + tick2) / 3 + tick3;
  EXPECT_EQ (complex, 75 * tick);
}

TEST (UnitsTest, TickTypeConversions)
{
  // Test that int64_t -> double conversion is value-preserving (allowed
  // implicitly)
  static_assert (mp_units::is_value_preserving<int64_t, double>);

  // Test that double -> int64_t conversion is NOT value-preserving (requires
  // explicit cast)
  static_assert (!mp_units::is_value_preserving<double, int64_t>);

  // Test conversion from tick_t to precise_tick_t (allowed: int64_t -> double)
  tick_t int_tick = 100 * tick;

  // Lambda that explicitly expects precise_tick_t
  auto precise_lambda = [] (precise_tick_t precise) -> double {
    return precise.numerical_value_in (tick);
  };

  // Should be able to pass tick_t to function expecting precise_tick_t
  // (implicit conversion from int64_t to double is value-preserving)
  double result = precise_lambda (int_tick);
  EXPECT_DOUBLE_EQ (result, 100.0);

  // Test explicit conversion using value_cast for value-truncating conversions
  precise_tick_t double_tick = 100.5 * tick;

  // Convert precise_tick_t to tick_t using explicit cast (value-truncating)
  tick_t truncated_tick = value_cast<int64_t> (double_tick);
  EXPECT_EQ (truncated_tick.numerical_value_in (tick), 100);

  // Test with exact value that should not truncate
  precise_tick_t exact_tick = 200.0 * tick;
  tick_t         exact_converted = value_cast<int64_t> (exact_tick);
  EXPECT_EQ (exact_converted.numerical_value_in (tick), 200);

  // Test force_in method (alternative to value_cast)
  tick_t force_converted = double_tick.force_in<int64_t> (tick);
  EXPECT_EQ (force_converted.numerical_value_in (tick), 100);
}

TEST (UnitsTest, MixedTypeArithmetic)
{
  // Test arithmetic operations between tick_t and precise_tick_t
  tick_t         int_tick = 100 * tick;
  precise_tick_t double_tick = 50.5 * tick;

  // Addition: precise_tick_t + tick_t should yield precise_tick_t
  auto sum1 = double_tick + int_tick;
  static_assert (std::is_same_v<decltype (sum1), precise_tick_t>);
  EXPECT_DOUBLE_EQ (sum1.numerical_value_in (tick), 150.5);

  // Addition: tick_t + precise_tick_t should yield precise_tick_t
  auto sum2 = int_tick + double_tick;
  static_assert (std::is_same_v<decltype (sum2), precise_tick_t>);
  EXPECT_DOUBLE_EQ (sum2.numerical_value_in (tick), 150.5);

  // Subtraction: precise_tick_t - tick_t should yield precise_tick_t
  auto diff1 = double_tick - int_tick;
  static_assert (std::is_same_v<decltype (diff1), precise_tick_t>);
  EXPECT_DOUBLE_EQ (diff1.numerical_value_in (tick), -49.5);

  // Subtraction: tick_t - precise_tick_t should yield precise_tick_t
  auto diff2 = int_tick - double_tick;
  static_assert (std::is_same_v<decltype (diff2), precise_tick_t>);
  EXPECT_DOUBLE_EQ (diff2.numerical_value_in (tick), 49.5);

  // Multiplication by scalar: int_tick * double yields precise_tick_t
  auto multiplied_int = int_tick * 2.5;
  static_assert (std::is_same_v<decltype (multiplied_int), precise_tick_t>);
  EXPECT_DOUBLE_EQ (multiplied_int.numerical_value_in (tick), 250.0);

  // Multiplication by scalar: double_tick * int yields precise_tick_t
  auto multiplied_double = double_tick * 2;
  static_assert (std::is_same_v<decltype (multiplied_double), precise_tick_t>);
  EXPECT_DOUBLE_EQ (multiplied_double.numerical_value_in (tick), 101.0);
}

TEST (UnitsTest, ExplicitConversionRequired)
{
  // Test that explicit conversion is required for value-truncating conversions
  precise_tick_t double_tick = 100.5 * tick;

  // This demonstrates that we cannot implicitly convert precise_tick_t to
  // tick_t The following line would fail to compile if uncommented: tick_t
  // int_tick = double_tick; // Should fail - requires explicit conversion

  // But we can use explicit conversion methods
  tick_t explicit_converted = value_cast<int64_t> (double_tick);
  EXPECT_EQ (explicit_converted.numerical_value_in (tick), 100);

  // Test with force_in method
  tick_t force_converted = double_tick.force_in<int64_t> (tick);
  EXPECT_EQ (force_converted.numerical_value_in (tick), 100);
}
