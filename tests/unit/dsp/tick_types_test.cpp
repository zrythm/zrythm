// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tick_types.h"

#include <gtest/gtest.h>

namespace zrythm::dsp::tests
{

TEST (TickTypesTest, DefaultConstruction)
{
  TimelineTick tl;
  ContentTick  ct;
  EXPECT_DOUBLE_EQ (tl.asDouble (), 0.0);
  EXPECT_DOUBLE_EQ (ct.asDouble (), 0.0);
}

TEST (TickTypesTest, ConstructionFromQuantity)
{
  TimelineTick tl{ units::ticks (1920.0) };
  ContentTick  ct{ units::ticks (960.0) };
  EXPECT_DOUBLE_EQ (tl.asDouble (), 1920.0);
  EXPECT_DOUBLE_EQ (ct.asDouble (), 960.0);
}

TEST (TickTypesTest, SameDomainAddition)
{
  TimelineTick a{ units::ticks (100.0) };
  TimelineTick b{ units::ticks (200.0) };
  TimelineTick c = a + b;
  EXPECT_DOUBLE_EQ (c.asDouble (), 300.0);
}

TEST (TickTypesTest, SameDomainSubtraction)
{
  ContentTick a{ units::ticks (500.0) };
  ContentTick b{ units::ticks (200.0) };
  ContentTick c = a - b;
  EXPECT_DOUBLE_EQ (c.asDouble (), 300.0);
}

TEST (TickTypesTest, ComparisonSameDomain)
{
  TimelineTick a{ units::ticks (100.0) };
  TimelineTick b{ units::ticks (200.0) };
  EXPECT_TRUE (a < b);
  EXPECT_TRUE (b > a);
  EXPECT_TRUE (a != b);
  EXPECT_FALSE (a == b);
}

TEST (TickTypesTest, AbsMin)
{
  ContentTick neg{ units::ticks (-50.0) };
  EXPECT_DOUBLE_EQ (abs (neg).asDouble (), 50.0);

  ContentTick a{ units::ticks (10.0) };
  ContentTick b{ units::ticks (20.0) };
  EXPECT_DOUBLE_EQ (std::min (a, b).asDouble (), 10.0);
  EXPECT_DOUBLE_EQ (std::max (a, b).asDouble (), 20.0);
}

TEST (TickTypesTest, AsQuantity)
{
  TimelineTick tl{ units::ticks (960.0) };
  EXPECT_DOUBLE_EQ (tl.asQuantity ().in (units::ticks), 960.0);
}

// --- Compile-time type safety: cross-domain operations must be ill-formed ---

template <typename A, typename B>
concept CrossAddable = requires (A a, B b) {
  { a + b };
};

template <typename A, typename B>
concept CrossComparable = requires (A a, B b) {
  { a < b } -> std::convertible_to<bool>;
};

// Same-domain operations MUST work.
static_assert (CrossAddable<TimelineTick, TimelineTick>);
static_assert (CrossAddable<ContentTick, ContentTick>);
static_assert (CrossComparable<TimelineTick, TimelineTick>);
static_assert (CrossComparable<ContentTick, ContentTick>);

// Cross-domain operations MUST NOT work.
static_assert (!CrossAddable<TimelineTick, ContentTick>);
static_assert (!CrossAddable<ContentTick, TimelineTick>);
static_assert (!CrossComparable<TimelineTick, ContentTick>);
static_assert (!CrossComparable<ContentTick, TimelineTick>);

// --- Integer variant and cross-precision conversion tests ---

TEST (TickTypesTest, IntegerVariantConstruction)
{
  TimelineTickI ti{ units::ticks (960) };
  ContentTickI  ci{ units::ticks (480) };
  EXPECT_EQ (ti.asDouble (), 960.0);
  EXPECT_EQ (ci.asDouble (), 480.0);
}

TEST (TickTypesTest, ImplicitWideningIntToDouble)
{
  TimelineTickI ti{ units::ticks (100) };
  TimelineTick  td = ti; // implicit widening
  EXPECT_DOUBLE_EQ (td.asDouble (), 100.0);
}

TEST (TickTypesTest, ContentTickIntegerVariant)
{
  ContentTickI ci{ units::ticks (500) };
  ContentTick  cd{ units::ticks (500.0) };
  EXPECT_EQ (ci, cd); // implicit widening for comparison
}

// --- Formatting tests ---

TEST (TickTypesTest, FormatTimelineTick)
{
  TimelineTick t{ units::ticks (1920.0) };
  EXPECT_EQ (fmt::format ("{}", t), "1920 timeline ticks");
}

TEST (TickTypesTest, FormatContentTick)
{
  ContentTick c{ units::ticks (960.0) };
  EXPECT_EQ (fmt::format ("{}", c), "960 content ticks");
}

TEST (TickTypesTest, FormatWithSpec)
{
  TimelineTick t{ units::ticks (3.14159) };
  EXPECT_EQ (fmt::format ("{:.2f}", t), "3.14 timeline ticks");
}

TEST (TickTypesTest, FormatIntegerVariant)
{
  TimelineTickI ti{ units::ticks (480) };
  EXPECT_EQ (fmt::format ("{}", ti), "480 timeline ticks");
  ContentTickI ci{ units::ticks (240) };
  EXPECT_EQ (fmt::format ("{}", ci), "240 content ticks");
}

} // namespace zrythm::dsp::tests
