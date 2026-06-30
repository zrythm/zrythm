// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/time_warp_map.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

namespace
{

TimeWarpMap
make_identity_map (int64_t length)
{
  TimeWarpMap map;
  map.source_length = units::samples (length);
  map.output_length = units::samples (length);
  map.anchors.push_back ({ units::samples (0), units::samples (0) });
  map.anchors.push_back ({ units::samples (length), units::samples (length) });
  return map;
}

} // namespace

TEST (TimeWarpMapTest, EmptyMapIsInvalid)
{
  TimeWarpMap map;
  EXPECT_FALSE (map.is_valid ());
}

TEST (TimeWarpMapTest, SingleAnchorIsInvalid)
{
  TimeWarpMap map;
  map.source_length = units::samples (100);
  map.output_length = units::samples (100);
  map.anchors.push_back ({ units::samples (0), units::samples (0) });
  EXPECT_FALSE (map.is_valid ());
}

TEST (TimeWarpMapTest, ZeroLengthIsInvalid)
{
  TimeWarpMap map;
  map.source_length = units::samples (0);
  map.output_length = units::samples (0);
  map.anchors.push_back ({ units::samples (0), units::samples (0) });
  map.anchors.push_back ({ units::samples (0), units::samples (0) });
  EXPECT_FALSE (map.is_valid ());
}

TEST (TimeWarpMapTest, IdentityMapIsValid)
{
  const auto map = make_identity_map (1000);
  ASSERT_TRUE (map.is_valid ());
  EXPECT_EQ (map.anchors.size (), 2u);
}

TEST (TimeWarpMapTest, WrongEndpointsAreInvalid)
{
  auto map = make_identity_map (1000);
  // Tamper with the start anchor.
  map.anchors.front ().source_frame = units::samples (1);
  EXPECT_FALSE (map.is_valid ());
}

TEST (TimeWarpMapTest, UnsortedSourceFramesAreInvalid)
{
  TimeWarpMap map;
  map.source_length = units::samples (1000);
  map.output_length = units::samples (2000);
  map.anchors.push_back ({ units::samples (0), units::samples (0) });
  map.anchors.push_back ({ units::samples (700), units::samples (1400) });
  // Source frame 500 comes after 700 -> unsorted.
  map.anchors.push_back ({ units::samples (500), units::samples (1000) });
  map.anchors.push_back ({ units::samples (1000), units::samples (2000) });
  EXPECT_FALSE (map.is_valid ());
}

TEST (TimeWarpMapTest, NonIncreasingOutputFramesAreInvalid)
{
  TimeWarpMap map;
  map.source_length = units::samples (100);
  map.output_length = units::samples (100);
  map.anchors.push_back ({ units::samples (0), units::samples (0) });
  // Output 90 then 50 -> non-increasing while source keeps increasing.
  map.anchors.push_back ({ units::samples (30), units::samples (90) });
  map.anchors.push_back ({ units::samples (60), units::samples (50) });
  map.anchors.push_back ({ units::samples (100), units::samples (100) });
  EXPECT_FALSE (map.is_valid ());
}

TEST (TimeWarpMapTest, MultiAnchorMonotonicIsValid)
{
  TimeWarpMap map;
  map.source_length = units::samples (1000);
  map.output_length = units::samples (2000);
  map.anchors.push_back ({ units::samples (0), units::samples (0) });
  map.anchors.push_back ({ units::samples (500), units::samples (1000) });
  map.anchors.push_back ({ units::samples (1000), units::samples (2000) });
  EXPECT_TRUE (map.is_valid ());
}

} // namespace zrythm::dsp
