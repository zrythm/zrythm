// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <string>
#include <thread>
#include <vector>

#include "plugins/lv2_urid_map.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::plugins
{

TEST (Lv2UridMapTest, SameUriReturnsSameNonZeroUrid)
{
  Lv2UridMap map;
  const auto first = map.map ("http://example.net/one");
  ASSERT_NE (first, 0u);
  EXPECT_EQ (map.map ("http://example.net/one"), first);
}

TEST (Lv2UridMapTest, DistinctUrisGetDistinctUrids)
{
  Lv2UridMap map;
  const auto first = map.map ("http://example.net/one");
  const auto second = map.map ("http://example.net/two");
  EXPECT_NE (first, second);
}

TEST (Lv2UridMapTest, UnmapRoundTripsTheOriginalUri)
{
  Lv2UridMap map;
  const auto urid = map.map ("http://example.net/one");
  ASSERT_NE (urid, 0u);
  EXPECT_STREQ (map.unmap (urid), "http://example.net/one");
}

TEST (Lv2UridMapTest, UnmapOfUnknownUridReturnsNullptr)
{
  Lv2UridMap map;
  map.map ("http://example.net/one");
  EXPECT_EQ (map.unmap (999999), nullptr);
}

TEST (Lv2UridMapTest, EmptyUriReturnsZero)
{
  Lv2UridMap map;
  EXPECT_EQ (map.map (""), 0u);
}

TEST (Lv2UridMapTest, MapGrowsWithoutBound)
{
  // Plugins may map arbitrarily many URIs over the process lifetime and
  // the spec requires the map to be dynamic, so any fixed capacity is a
  // contract violation.
  Lv2UridMap               map;
  std::vector<std::string> uris;
  uris.reserve (2000);
  for (int i = 0; i < 2000; ++i)
    {
      uris.push_back ("http://example.net/urid/" + std::to_string (i));
    }
  for (const auto &uri : uris)
    {
      EXPECT_NE (map.map (uri.c_str ()), 0u) << uri;
    }
  for (const auto &uri : uris)
    {
      EXPECT_STREQ (map.unmap (map.map (uri.c_str ())), uri.c_str ());
    }
}

TEST (Lv2UridMapTest, ConcurrentMappingIsConsistent)
{
  Lv2UridMap                map;
  constexpr int             per_thread = 500;
  std::vector<std::jthread> threads;
  for (int t = 0; t < 2; ++t)
    {
      threads.emplace_back ([&, t] {
        for (int i = 0; i < per_thread; ++i)
          {
            // every thread maps a shared URI and a thread-unique URI
            const auto shared = map.map ("http://example.net/shared");
            const auto unique = map.map (
              ("http://example.net/t" + std::to_string (t) + "/"
               + std::to_string (i))
                .c_str ());
            if (shared == 0u || unique == 0u)
              {
                ADD_FAILURE () << "map returned 0";
              }
          }
      });
    }
  threads.clear ();

  // all mappings resolve to the same URI they were created with
  for (int t = 0; t < 2; ++t)
    {
      for (int i = 0; i < per_thread; ++i)
        {
          const auto uri =
            "http://example.net/t" + std::to_string (t) + "/"
            + std::to_string (i);
          EXPECT_STREQ (map.unmap (map.map (uri.c_str ())), uri.c_str ());
        }
    }
}

TEST (Lv2UridMapTest, SharedInstanceCachesHostUrids)
{
  const auto &urids = lv2_host_urids ();
  EXPECT_NE (urids.atom_Sequence, 0u);
  EXPECT_NE (urids.midi_MidiEvent, 0u);
  EXPECT_NE (urids.time_Position, 0u);
  // host URIDs are stable and identical on repeated access
  EXPECT_EQ (lv2_host_urids ().atom_Sequence, urids.atom_Sequence);
  EXPECT_STREQ (
    Lv2UridMap::instance ().unmap (urids.atom_Sequence),
    "http://lv2plug.in/ns/ext/atom#Sequence");
}

} // namespace zrythm::plugins
