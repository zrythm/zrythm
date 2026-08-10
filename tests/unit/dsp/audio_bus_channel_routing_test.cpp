// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>
#include <vector>

#include "dsp/audio_bus_channel_routing.h"

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

class AudioBusChannelRoutingTest : public ::testing::Test
{
protected:
  struct Contribution
  {
    unsigned destination_channel{};
    unsigned source_channel{};
    float    gain{};

    bool operator== (const Contribution &) const = default;
  };

  static std::vector<Contribution> collect (
    const AudioBusChannelRouting &routing,
    const SpeakerArrangement     &src,
    const SpeakerArrangement     &dest)
  {
    std::vector<Contribution> contributions;
    routing.for_each_route (
      src, dest, [&] (unsigned dest_ch, unsigned src_ch, float gain) {
        contributions.push_back ({ dest_ch, src_ch, gain });
      });
    return contributions;
  }
};

TEST_F (AudioBusChannelRoutingTest, DefaultIsDerived)
{
  const AudioBusChannelRouting routing;
  EXPECT_TRUE (routing.is_derived ());
  EXPECT_TRUE (routing.routes ().empty ());
}

TEST_F (AudioBusChannelRoutingTest, DerivedMatchingCountsRouteChannelForChannel)
{
  const auto contributions = collect (
    AudioBusChannelRouting{}, SpeakerArrangement::stereo (),
    SpeakerArrangement::stereo ());
  ASSERT_EQ (contributions.size (), 2);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
  EXPECT_EQ (contributions[1], (Contribution{ 1, 1, 1.f }));
}

TEST_F (
  AudioBusChannelRoutingTest,
  DerivedSingleChannelSourceFeedsEveryDestination)
{
  for (
    const auto &src :
    { SpeakerArrangement::mono (), SpeakerArrangement::discrete_channels (1) })
    {
      const auto contributions =
        collect (AudioBusChannelRouting{}, src, SpeakerArrangement::stereo ());
      ASSERT_EQ (contributions.size (), 2);
      EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
      EXPECT_EQ (contributions[1], (Contribution{ 1, 0, 1.f }));
    }
}

TEST_F (AudioBusChannelRoutingTest, DerivedPairSumsIntoSingleChannelDestination)
{
  const auto contributions = collect (
    AudioBusChannelRouting{}, SpeakerArrangement::stereo (),
    SpeakerArrangement::mono ());
  ASSERT_EQ (contributions.size (), 2);
  EXPECT_EQ (contributions[0].destination_channel, 0);
  EXPECT_EQ (contributions[0].source_channel, 0);
  EXPECT_EQ (contributions[1].destination_channel, 0);
  EXPECT_EQ (contributions[1].source_channel, 1);
  EXPECT_LT (contributions[0].gain, 1.f);
  EXPECT_LT (contributions[1].gain, 1.f);
}

TEST_F (
  AudioBusChannelRoutingTest,
  DerivedDropsSourceChannelsBeyondDestinationCount)
{
  const auto contributions = collect (
    AudioBusChannelRouting{}, SpeakerArrangement::discrete_channels (4),
    SpeakerArrangement::discrete_channels (2));
  ASSERT_EQ (contributions.size (), 2);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
  EXPECT_EQ (contributions[1], (Contribution{ 1, 1, 1.f }));
}

TEST_F (AudioBusChannelRoutingTest, ExplicitRoutesReplaceDerivedOnes)
{
  const AudioBusChannelRouting routing{
    { { .source_channel = 1, .destination_channel = 0, .gain = 0.5f } }
  };
  EXPECT_FALSE (routing.is_derived ());

  const auto contributions = collect (
    routing, SpeakerArrangement::stereo (), SpeakerArrangement::stereo ());
  ASSERT_EQ (contributions.size (), 1);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 1, 0.5f }));
}

TEST_F (AudioBusChannelRoutingTest, EmptyExplicitRoutingPassesNothing)
{
  const AudioBusChannelRouting routing{ std::vector<AudioBusChannelRoute>{} };
  EXPECT_FALSE (routing.is_derived ());
  EXPECT_TRUE (
    collect (
      routing, SpeakerArrangement::stereo (), SpeakerArrangement::stereo ())
      .empty ());
}

TEST_F (AudioBusChannelRoutingTest, RoutesOutsideEitherArrangementAreSkipped)
{
  const AudioBusChannelRouting routing{
    { { .source_channel = 0, .destination_channel = 0 },
     { .source_channel = 7, .destination_channel = 0 },
     { .source_channel = 0, .destination_channel = 7 } }
  };

  const auto contributions = collect (
    routing, SpeakerArrangement::stereo (), SpeakerArrangement::stereo ());
  ASSERT_EQ (contributions.size (), 1);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
}

TEST_F (AudioBusChannelRoutingTest, DuplicateRoutesRejected)
{
  EXPECT_THROW (
    (AudioBusChannelRouting{
      { { .source_channel = 1, .destination_channel = 2 },
       { .source_channel = 1, .destination_channel = 2 } }
  }),
    std::invalid_argument);
}

TEST_F (AudioBusChannelRoutingTest, ChannelForChannelDetection)
{
  const AudioBusChannelRouting derived;
  EXPECT_TRUE (derived.is_channel_for_channel (
    SpeakerArrangement::stereo (), SpeakerArrangement::stereo ()));
  EXPECT_FALSE (derived.is_channel_for_channel (
    SpeakerArrangement::mono (), SpeakerArrangement::stereo ()));

  const AudioBusChannelRouting identity{
    { { .source_channel = 0, .destination_channel = 0 },
     { .source_channel = 1, .destination_channel = 1 } }
  };
  EXPECT_TRUE (identity.is_channel_for_channel (
    SpeakerArrangement::stereo (), SpeakerArrangement::stereo ()));

  const AudioBusChannelRouting swapped{
    { { .source_channel = 1, .destination_channel = 0 },
     { .source_channel = 0, .destination_channel = 1 } }
  };
  EXPECT_FALSE (swapped.is_channel_for_channel (
    SpeakerArrangement::stereo (), SpeakerArrangement::stereo ()));

  const AudioBusChannelRouting attenuated{
    { { .source_channel = 0, .destination_channel = 0, .gain = 0.5f },
     { .source_channel = 1, .destination_channel = 1 } }
  };
  EXPECT_FALSE (attenuated.is_channel_for_channel (
    SpeakerArrangement::stereo (), SpeakerArrangement::stereo ()));
}

TEST_F (AudioBusChannelRoutingTest, JsonRoundtrip)
{
  const auto routings = {
    AudioBusChannelRouting{},
    AudioBusChannelRouting{ std::vector<AudioBusChannelRoute>{} },
    AudioBusChannelRouting{
      { { .source_channel = 1, .destination_channel = 0, .gain = 0.25f },
        { .source_channel = 0, .destination_channel = 1 } } },
  };
  for (const auto &routing : routings)
    {
      nlohmann::json         j;
      AudioBusChannelRouting deserialized;
      to_json (j, routing);
      from_json (j, deserialized);
      EXPECT_EQ (deserialized, routing);
    }
}

TEST_F (AudioBusChannelRoutingTest, JsonRejectsDuplicateRoutes)
{
  const auto j = nlohmann::json{
    { "routes",
     nlohmann::json::array (
        { nlohmann::json{
            { "sourceChannel", 0 }, { "destinationChannel", 0 }, { "gain", 1.f } },
          nlohmann::json{
            { "sourceChannel", 0 }, { "destinationChannel", 0 }, { "gain", 1.f } } }) }
  };
  AudioBusChannelRouting routing;
  EXPECT_THROW (from_json (j, routing), std::invalid_argument);
}

TEST_F (AudioBusChannelRoutingTest, Formatting)
{
  EXPECT_EQ (fmt::format ("{}", AudioBusChannelRouting{}), "derived");
  EXPECT_EQ (
    fmt::format (
      "{}",
      AudioBusChannelRouting{
        { { .source_channel = 1, .destination_channel = 0, .gain = 0.5f } } }),
    "[1->0@0.50]");
}

} // namespace zrythm::dsp
