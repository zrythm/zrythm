// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

#include "dsp/audio_bus_channel_routing.h"
#include "utils/views.h"

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
  DerivedSingleChannelDiscreteSourceFeedsEveryDestination)
{
  const auto contributions = collect (
    AudioBusChannelRouting{}, SpeakerArrangement::discrete_channels (1),
    SpeakerArrangement::stereo ());
  ASSERT_EQ (contributions.size (), 2);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
  EXPECT_EQ (contributions[1], (Contribution{ 1, 0, 1.f }));
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

// Speaker masks used below (bit order = channel order):
// 5.1 = L R C LFE Ls Rs (channels 0-5)
static constexpr auto k51 = SpeakerArrangement::from_speaker_bits (
  std::to_underlying (SpeakerArrangement::Speaker::Left)
  | std::to_underlying (SpeakerArrangement::Speaker::Right)
  | std::to_underlying (SpeakerArrangement::Speaker::Center)
  | std::to_underlying (SpeakerArrangement::Speaker::Lfe)
  | std::to_underlying (SpeakerArrangement::Speaker::LeftSurround)
  | std::to_underlying (SpeakerArrangement::Speaker::RightSurround));
// quad = L R Ls Rs
static constexpr auto kQuad = SpeakerArrangement::from_speaker_bits (
  std::to_underlying (SpeakerArrangement::Speaker::Left)
  | std::to_underlying (SpeakerArrangement::Speaker::Right)
  | std::to_underlying (SpeakerArrangement::Speaker::LeftSurround)
  | std::to_underlying (SpeakerArrangement::Speaker::RightSurround));
// 7.1 (side fill) = 5.1 + side left/right (channels 6-7)
static constexpr auto k71SideFill = SpeakerArrangement::from_speaker_bits (
  std::to_underlying (SpeakerArrangement::Speaker::Left)
  | std::to_underlying (SpeakerArrangement::Speaker::Right)
  | std::to_underlying (SpeakerArrangement::Speaker::Center)
  | std::to_underlying (SpeakerArrangement::Speaker::Lfe)
  | std::to_underlying (SpeakerArrangement::Speaker::LeftSurround)
  | std::to_underlying (SpeakerArrangement::Speaker::RightSurround)
  | std::to_underlying (SpeakerArrangement::Speaker::SideLeft)
  | std::to_underlying (SpeakerArrangement::Speaker::SideRight));
// 3.1 = L R C LFE (channels 0-3)
static constexpr auto k31 = SpeakerArrangement::from_speaker_bits (
  std::to_underlying (SpeakerArrangement::Speaker::Left)
  | std::to_underlying (SpeakerArrangement::Speaker::Right)
  | std::to_underlying (SpeakerArrangement::Speaker::Center)
  | std::to_underlying (SpeakerArrangement::Speaker::Lfe));
// 6.1 = 5.1 + surround center (channel 6)
static constexpr auto k61 = SpeakerArrangement::from_speaker_bits (
  k51.speaker_bits ()
  | std::to_underlying (SpeakerArrangement::Speaker::SurroundCenter));
// 5.1 (side fill) = L R C LFE Sl Sr (channels 0-5)
static constexpr auto k51SideFill = SpeakerArrangement::from_speaker_bits (
  std::to_underlying (SpeakerArrangement::Speaker::Left)
  | std::to_underlying (SpeakerArrangement::Speaker::Right)
  | std::to_underlying (SpeakerArrangement::Speaker::Center)
  | std::to_underlying (SpeakerArrangement::Speaker::Lfe)
  | std::to_underlying (SpeakerArrangement::Speaker::SideLeft)
  | std::to_underlying (SpeakerArrangement::Speaker::SideRight));
// 5.0 with a left surround only (channels 0-4)
static constexpr auto k50SingleSidedSurround =
  SpeakerArrangement::from_speaker_bits (
    std::to_underlying (SpeakerArrangement::Speaker::Left)
    | std::to_underlying (SpeakerArrangement::Speaker::Right)
    | std::to_underlying (SpeakerArrangement::Speaker::Center)
    | std::to_underlying (SpeakerArrangement::Speaker::LeftSurround));

static constexpr float kFoldGain = 0.70710678f;

TEST_F (AudioBusChannelRoutingTest, DerivedMonoToStereoFoldsToFrontPair)
{
  const auto contributions = collect (
    AudioBusChannelRouting{}, SpeakerArrangement::mono (),
    SpeakerArrangement::stereo ());
  ASSERT_EQ (contributions.size (), 2);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, kFoldGain }));
  EXPECT_EQ (contributions[1], (Contribution{ 1, 0, kFoldGain }));
}

TEST_F (AudioBusChannelRoutingTest, DerivedMonoToSurroundFeedsCenterAtUnity)
{
  const auto contributions =
    collect (AudioBusChannelRouting{}, SpeakerArrangement::mono (), k51);
  ASSERT_EQ (contributions.size (), 1);
  EXPECT_EQ (contributions[0], (Contribution{ 2, 0, 1.f }));
}

TEST_F (
  AudioBusChannelRoutingTest,
  DerivedSurroundToMonoSumsFrontPairAndCenterAndDropsSurrounds)
{
  const auto front_gains =
    calculate_panning (PanLaw::Minus6dB, PanAlgorithm::SquareRoot, 0.5f);
  const auto contributions =
    collect (AudioBusChannelRouting{}, k51, SpeakerArrangement::mono ());
  ASSERT_EQ (contributions.size (), 3);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, front_gains.first }));
  EXPECT_EQ (contributions[1], (Contribution{ 0, 1, front_gains.second }));
  // center folds in at -3dB relative to the front pair
  EXPECT_EQ (
    contributions[2], (Contribution{ 0, 2, front_gains.first * kFoldGain }));
  // LFE (source channel 3) is intentionally discarded, surrounds (source
  // channels 4-5) are dropped
  EXPECT_TRUE (std::ranges::none_of (contributions, [] (const auto &c) {
    return c.source_channel >= 3;
  }));
}

TEST_F (AudioBusChannelRoutingTest, DerivedQuadTo31FoldsSurroundsIntoFrontPair)
{
  const auto contributions = collect (AudioBusChannelRouting{}, kQuad, k31);
  ASSERT_EQ (contributions.size (), 4);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
  EXPECT_EQ (contributions[1], (Contribution{ 1, 1, 1.f }));
  // surrounds fold to the same-side front at -3dB instead of landing on the
  // center/LFE channels of the same index
  EXPECT_EQ (contributions[2], (Contribution{ 0, 2, kFoldGain }));
  EXPECT_EQ (contributions[3], (Contribution{ 1, 3, kFoldGain }));
}

TEST_F (AudioBusChannelRoutingTest, Derived31ToQuadFoldsCenterAndDropsLfe)
{
  const auto contributions = collect (AudioBusChannelRouting{}, k31, kQuad);
  ASSERT_EQ (contributions.size (), 4);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
  EXPECT_EQ (contributions[1], (Contribution{ 1, 1, 1.f }));
  // center folds to the front pair at -3dB
  EXPECT_EQ (contributions[2], (Contribution{ 0, 2, kFoldGain }));
  EXPECT_EQ (contributions[3], (Contribution{ 1, 2, kFoldGain }));
}

TEST_F (AudioBusChannelRoutingTest, Derived61To51FoldsRearCenterIntoSurroundPair)
{
  const auto contributions = collect (AudioBusChannelRouting{}, k61, k51);
  ASSERT_EQ (contributions.size (), 8);
  for (const auto ch : std::views::iota (0u, 6u))
    {
      EXPECT_EQ (contributions[ch], (Contribution{ ch, ch, 1.f }));
    }
  // rear center folds into the surround pair at -3dB
  EXPECT_EQ (contributions[6], (Contribution{ 4, 6, kFoldGain }));
  EXPECT_EQ (contributions[7], (Contribution{ 5, 6, kFoldGain }));
}

TEST_F (
  AudioBusChannelRoutingTest,
  Derived61To51SideFillFoldsRearCenterIntoSideSurrounds)
{
  const auto contributions =
    collect (AudioBusChannelRouting{}, k61, k51SideFill);
  ASSERT_EQ (contributions.size (), 8);
  // L R C LFE route to themselves at unity
  for (const auto ch : std::views::iota (0u, 4u))
    {
      EXPECT_EQ (contributions[ch], (Contribution{ ch, ch, 1.f }));
    }
  // rear surrounds fold into the side surrounds at -3dB
  EXPECT_EQ (contributions[4], (Contribution{ 4, 4, kFoldGain }));
  EXPECT_EQ (contributions[5], (Contribution{ 5, 5, kFoldGain }));
  // rear center folds into the side surrounds at -3dB
  EXPECT_EQ (contributions[6], (Contribution{ 4, 6, kFoldGain }));
  EXPECT_EQ (contributions[7], (Contribution{ 5, 6, kFoldGain }));
}

TEST_F (
  AudioBusChannelRoutingTest,
  DerivedRearCenterFallsBackToCenterWhenSurroundsAreSingleSided)
{
  const auto contributions =
    collect (AudioBusChannelRouting{}, k61, k50SingleSidedSurround);
  ASSERT_EQ (contributions.size (), 6);
  // L R C route to themselves at unity
  for (const auto ch : std::views::iota (0u, 3u))
    {
      EXPECT_EQ (contributions[ch], (Contribution{ ch, ch, 1.f }));
    }
  // LFE (source channel 3) is intentionally discarded
  // left surround routes to itself at unity
  EXPECT_EQ (contributions[3], (Contribution{ 3, 4, 1.f }));
  // right surround folds to the front right at -3dB
  EXPECT_EQ (contributions[4], (Contribution{ 1, 5, kFoldGain }));
  // rear center folds to the front center at -3dB rather than pulling the
  // image to the only surround side
  EXPECT_EQ (contributions[5], (Contribution{ 2, 6, kFoldGain }));
}

TEST_F (
  AudioBusChannelRoutingTest,
  DerivedSurroundToStereoFoldsCenterAndSurroundsAndDropsLfe)
{
  const auto contributions =
    collect (AudioBusChannelRouting{}, k51, SpeakerArrangement::stereo ());
  ASSERT_EQ (contributions.size (), 6);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
  EXPECT_EQ (contributions[1], (Contribution{ 1, 1, 1.f }));
  // center folds to both at -3dB
  EXPECT_EQ (contributions[2], (Contribution{ 0, 2, kFoldGain }));
  EXPECT_EQ (contributions[3], (Contribution{ 1, 2, kFoldGain }));
  // surrounds fold to the same-side front at -3dB
  EXPECT_EQ (contributions[4], (Contribution{ 0, 4, kFoldGain }));
  EXPECT_EQ (contributions[5], (Contribution{ 1, 5, kFoldGain }));
  // LFE (source channel 3) is intentionally discarded
  EXPECT_TRUE (std::ranges::none_of (contributions, [] (const auto &c) {
    return c.source_channel == 3;
  }));
}

TEST_F (AudioBusChannelRoutingTest, DerivedStereoToSurroundOnlyFeedsFrontPair)
{
  const auto contributions =
    collect (AudioBusChannelRouting{}, SpeakerArrangement::stereo (), k51);
  ASSERT_EQ (contributions.size (), 2);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
  EXPECT_EQ (contributions[1], (Contribution{ 1, 1, 1.f }));
}

TEST_F (AudioBusChannelRoutingTest, DerivedQuadToStereoFoldsSurrounds)
{
  const auto contributions =
    collect (AudioBusChannelRouting{}, kQuad, SpeakerArrangement::stereo ());
  ASSERT_EQ (contributions.size (), 4);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
  EXPECT_EQ (contributions[1], (Contribution{ 1, 1, 1.f }));
  EXPECT_EQ (contributions[2], (Contribution{ 0, 2, kFoldGain }));
  EXPECT_EQ (contributions[3], (Contribution{ 1, 3, kFoldGain }));
}

TEST_F (
  AudioBusChannelRoutingTest,
  DerivedSurroundToQuadFoldsCenterAndKeepsSurrounds)
{
  const auto contributions = collect (AudioBusChannelRouting{}, k51, kQuad);
  ASSERT_EQ (contributions.size (), 6);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
  EXPECT_EQ (contributions[1], (Contribution{ 1, 1, 1.f }));
  // center folds to the front pair at -3dB
  EXPECT_EQ (contributions[2], (Contribution{ 0, 2, kFoldGain }));
  EXPECT_EQ (contributions[3], (Contribution{ 1, 2, kFoldGain }));
  // surrounds route to themselves at unity
  EXPECT_EQ (contributions[4], (Contribution{ 2, 4, 1.f }));
  EXPECT_EQ (contributions[5], (Contribution{ 3, 5, 1.f }));
}

TEST_F (
  AudioBusChannelRoutingTest,
  Derived71To51FoldsSideSurroundsIntoRearSurrounds)
{
  const auto contributions =
    collect (AudioBusChannelRouting{}, k71SideFill, k51);
  ASSERT_EQ (contributions.size (), 8);
  EXPECT_EQ (contributions[0], (Contribution{ 0, 0, 1.f }));
  EXPECT_EQ (contributions[1], (Contribution{ 1, 1, 1.f }));
  EXPECT_EQ (contributions[2], (Contribution{ 2, 2, 1.f }));
  // LFE routes to itself at unity since the destination has it
  EXPECT_EQ (contributions[3], (Contribution{ 3, 3, 1.f }));
  EXPECT_EQ (contributions[4], (Contribution{ 4, 4, 1.f }));
  EXPECT_EQ (contributions[5], (Contribution{ 5, 5, 1.f }));
  // side surrounds fold into the rear surrounds at -3dB
  EXPECT_EQ (contributions[6], (Contribution{ 4, 6, kFoldGain }));
  EXPECT_EQ (contributions[7], (Contribution{ 5, 7, kFoldGain }));
}

TEST_F (AudioBusChannelRoutingTest, Derived51To71LeavesSideSurroundsUnfed)
{
  const auto contributions =
    collect (AudioBusChannelRouting{}, k51, k71SideFill);
  ASSERT_EQ (contributions.size (), 6);
  for (const auto [i, contribution] : utils::views::enumerate (contributions))
    {
      EXPECT_EQ (
        contribution,
        (Contribution{
          static_cast<unsigned> (i), static_cast<unsigned> (i), 1.f }));
    }
}

TEST_F (AudioBusChannelRoutingTest, DerivedRoutingDropsContent)
{
  // speaker folding: only LFE is discarded, which is intentional
  EXPECT_FALSE (
    derived_routing_drops_content (k51, SpeakerArrangement::stereo ()));
  EXPECT_FALSE (
    derived_routing_drops_content (SpeakerArrangement::stereo (), k51));
  EXPECT_FALSE (
    derived_routing_drops_content (kQuad, SpeakerArrangement::stereo ()));
  EXPECT_FALSE (derived_routing_drops_content (k71SideFill, k51));
  EXPECT_FALSE (derived_routing_drops_content (k51, k51));
  EXPECT_FALSE (
    derived_routing_drops_content (SpeakerArrangement::mono (), k51));

  // stereo to mono sums the pair, nothing dropped
  EXPECT_FALSE (derived_routing_drops_content (
    SpeakerArrangement::stereo (), SpeakerArrangement::mono ()));

  // surround to mono keeps the front pair and center, dropping the
  // surrounds
  EXPECT_TRUE (derived_routing_drops_content (k51, SpeakerArrangement::mono ()));

  // count-based fallback drops channels beyond the destination's count
  EXPECT_TRUE (derived_routing_drops_content (
    SpeakerArrangement::discrete_channels (4),
    SpeakerArrangement::discrete_channels (2)));
  EXPECT_FALSE (derived_routing_drops_content (
    SpeakerArrangement::discrete_channels (2),
    SpeakerArrangement::discrete_channels (4)));

  // ambisonics mixed with any other kind has no honest mapping
  EXPECT_TRUE (derived_routing_drops_content (
    SpeakerArrangement::ambisonics (1), SpeakerArrangement::stereo ()));
  EXPECT_TRUE (derived_routing_drops_content (
    SpeakerArrangement::stereo (), SpeakerArrangement::ambisonics (1)));
  EXPECT_TRUE (derived_routing_drops_content (
    SpeakerArrangement::ambisonics (1),
    SpeakerArrangement::discrete_channels (4)));

  // ambisonics order reduction drops channels
  EXPECT_TRUE (derived_routing_drops_content (
    SpeakerArrangement::ambisonics (2), SpeakerArrangement::ambisonics (1)));

  // ambisonics convention mismatches mis-map channels
  EXPECT_TRUE (derived_routing_drops_content (
    SpeakerArrangement::ambisonics (1),
    SpeakerArrangement::ambisonics (
      1, SpeakerArrangement::AmbisonicOrdering::FuMa,
      SpeakerArrangement::AmbisonicNormalization::MaxN)));
  EXPECT_TRUE (derived_routing_drops_content (
    SpeakerArrangement::ambisonics (1),
    SpeakerArrangement::ambisonics (
      1, SpeakerArrangement::AmbisonicOrdering::Acn,
      SpeakerArrangement::AmbisonicNormalization::N3d)));
  EXPECT_FALSE (derived_routing_drops_content (
    SpeakerArrangement::ambisonics (1), SpeakerArrangement::ambisonics (1)));
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
  EXPECT_TRUE (derived.is_channel_for_channel (k51, k51));
  // equal counts but different speaker layouts fold per speaker
  EXPECT_FALSE (derived.is_channel_for_channel (kQuad, k31));

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
