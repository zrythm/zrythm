// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/expandable_tick_range.h"

#include <gtest/gtest.h>

namespace zrythm::utils
{

TEST (ExpandableTickRangeTest, DefaultConstruction)
{
  ExpandableTickRange range;

  // Default range should be full content
  EXPECT_TRUE (range.is_full_content ());
  EXPECT_FALSE (range.range ().has_value ());
}

TEST (ExpandableTickRangeTest, ConstructionWithRange)
{
  ExpandableTickRange range{ std::make_optional (std::make_pair (10.0, 20.0)) };

  // Should not be full content
  EXPECT_FALSE (range.is_full_content ());

  // Should have the provided range
  auto result_range = range.range ();
  EXPECT_TRUE (result_range.has_value ());
  EXPECT_EQ (result_range->first, 10.0);
  EXPECT_EQ (result_range->second, 20.0);
}

TEST (ExpandableTickRangeTest, ExpandWithRange)
{
  ExpandableTickRange range{ std::make_optional (std::make_pair (5.0, 15.0)) };

  // Expand with a range that overlaps
  range.expand ({ 10.0, 25.0 });

  auto result_range = range.range ();
  EXPECT_TRUE (result_range.has_value ());
  EXPECT_EQ (result_range->first, 5.0);   // min start
  EXPECT_EQ (result_range->second, 25.0); // max end
}

TEST (ExpandableTickRangeTest, ExpandWithNonOverlappingRange)
{
  ExpandableTickRange range{ std::make_optional (std::make_pair (5.0, 15.0)) };

  // Expand with a range that doesn't overlap
  range.expand ({ 20.0, 30.0 });

  auto result_range = range.range ();
  EXPECT_TRUE (result_range.has_value ());
  EXPECT_EQ (result_range->first, 5.0);   // min start
  EXPECT_EQ (result_range->second, 30.0); // max end
}

TEST (ExpandableTickRangeTest, ExpandWithSmallerRange)
{
  ExpandableTickRange range{ std::make_optional (std::make_pair (10.0, 20.0)) };

  // Expand with a range that's completely inside
  range.expand ({ 12.0, 18.0 });

  auto result_range = range.range ();
  EXPECT_TRUE (result_range.has_value ());
  EXPECT_EQ (result_range->first, 10.0);  // original start preserved
  EXPECT_EQ (result_range->second, 20.0); // original end preserved
}

TEST (ExpandableTickRangeTest, ExpandToFull)
{
  ExpandableTickRange range{ std::make_optional (std::make_pair (10.0, 20.0)) };

  // Expand to full content
  range.expand_to_full ();

  EXPECT_TRUE (range.is_full_content ());
  EXPECT_FALSE (range.range ().has_value ());
}

TEST (ExpandableTickRangeTest, ExpandWithFullContentRange)
{
  ExpandableTickRange range1{ std::make_optional (std::make_pair (10.0, 20.0)) };
  ExpandableTickRange range2;
  range2.expand_to_full (); // Make range2 full content

  // Expanding with full content should make range1 full content
  range1.expand (range2);

  EXPECT_TRUE (range1.is_full_content ());
  EXPECT_FALSE (range1.range ().has_value ());
}

TEST (ExpandableTickRangeTest, ExpandWithNormalRange)
{
  ExpandableTickRange range1{ std::make_optional (std::make_pair (10.0, 20.0)) };
  ExpandableTickRange range2{ std::make_optional (std::make_pair (5.0, 25.0)) };

  range1.expand (range2);

  auto result_range = range1.range ();
  EXPECT_TRUE (result_range.has_value ());
  EXPECT_EQ (result_range->first, 5.0);   // min start
  EXPECT_EQ (result_range->second, 25.0); // max end
}

TEST (ExpandableTickRangeTest, FormatAsFullContent)
{
  ExpandableTickRange range;
  auto                formatted = format_as (range);

  EXPECT_EQ (formatted, "AffectedTickRange: (full content)");
}

TEST (ExpandableTickRangeTest, FormatAsNormalRange)
{
  ExpandableTickRange range{ std::make_optional (std::make_pair (10.0, 20.0)) };
  auto                formatted = format_as (range);

  // Should contain the range values
  EXPECT_TRUE (formatted.find ("10") != std::string::npos);
  EXPECT_TRUE (formatted.find ("20") != std::string::npos);
  EXPECT_TRUE (formatted.find ("AffectedTickRange:") != std::string::npos);
}

TEST (ExpandableTickRangeTest, ConstructionWithNullopt)
{
  ExpandableTickRange range{ std::nullopt };

  // Should be full content when constructed with nullopt
  EXPECT_TRUE (range.is_full_content ());
  EXPECT_FALSE (range.range ().has_value ());
}

TEST (ExpandableTickRangeTest, ExpandFullContentWithRange)
{
  ExpandableTickRange range; // Full content by default

  // Trying to expand full content with a range should not change it
  range.expand ({ 10.0, 20.0 });

  EXPECT_TRUE (range.is_full_content ());
  EXPECT_FALSE (range.range ().has_value ());
}

TEST (ExpandableTickRangeTest, EdgeCaseZeroRange)
{
  ExpandableTickRange range{ std::make_optional (std::make_pair (0.0, 0.0)) };

  EXPECT_FALSE (range.is_full_content ());
  auto result_range = range.range ();
  EXPECT_TRUE (result_range.has_value ());
  EXPECT_EQ (result_range->first, 0.0);
  EXPECT_EQ (result_range->second, 0.0);
}

TEST (ExpandableTickRangeTest, MultipleExpansions)
{
  ExpandableTickRange range{ std::make_optional (std::make_pair (10.0, 20.0)) };

  // Multiple expansions
  range.expand ({ 5.0, 15.0 });
  range.expand ({ 25.0, 30.0 });
  range.expand ({ 0.0, 35.0 });

  auto result_range = range.range ();
  EXPECT_TRUE (result_range.has_value ());
  EXPECT_EQ (result_range->first, 0.0);   // min start from all expansions
  EXPECT_EQ (result_range->second, 35.0); // max end from all expansions
}
}
