// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>

#include "dsp/speaker_arrangement.h"

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

class SpeakerArrangementTest : public ::testing::Test
{
protected:
  using Speaker = SpeakerArrangement::Speaker;

  static constexpr auto speaker_bits (auto &&... speakers)
  {
    return (std::to_underlying (speakers) | ...);
  }
};

TEST_F (SpeakerArrangementTest, Mono)
{
  constexpr auto mono = SpeakerArrangement::mono ();
  EXPECT_EQ (mono.kind (), SpeakerArrangement::Kind::Speakers);
  EXPECT_EQ (mono.channel_count (), 1);
  EXPECT_TRUE (mono.is_mono ());
  EXPECT_FALSE (mono.is_stereo ());
  EXPECT_TRUE (mono.has_speaker (Speaker::Mono));
}

TEST_F (SpeakerArrangementTest, Stereo)
{
  constexpr auto stereo = SpeakerArrangement::stereo ();
  EXPECT_EQ (stereo.channel_count (), 2);
  EXPECT_TRUE (stereo.is_stereo ());
  EXPECT_FALSE (stereo.is_mono ());
  EXPECT_TRUE (stereo.has_speaker (Speaker::Left));
  EXPECT_TRUE (stereo.has_speaker (Speaker::Right));
  EXPECT_FALSE (stereo.has_speaker (Speaker::Center));
}

TEST_F (SpeakerArrangementTest, SurroundChannelCount)
{
  constexpr auto five_one = SpeakerArrangement::from_speaker_bits (speaker_bits (
    Speaker::Left, Speaker::Right, Speaker::Center, Speaker::Lfe,
    Speaker::LeftSurround, Speaker::RightSurround));
  EXPECT_EQ (five_one.channel_count (), 6);
  EXPECT_FALSE (five_one.is_mono ());
  EXPECT_FALSE (five_one.is_stereo ());
}

TEST_F (SpeakerArrangementTest, Ambisonics)
{
  constexpr auto foa = SpeakerArrangement::ambisonics (1);
  EXPECT_EQ (foa.kind (), SpeakerArrangement::Kind::Ambisonics);
  EXPECT_EQ (foa.channel_count (), 4);
  EXPECT_EQ (foa.ambisonic_order (), 1);

  constexpr auto seventh = SpeakerArrangement::ambisonics (7);
  EXPECT_EQ (seventh.channel_count (), 64);
}

TEST_F (SpeakerArrangementTest, AmbisonicsDefaultsToAmbiX)
{
  constexpr auto foa = SpeakerArrangement::ambisonics (1);
  EXPECT_EQ (
    foa.ambisonic_ordering (), SpeakerArrangement::AmbisonicOrdering::Acn);
  EXPECT_EQ (
    foa.ambisonic_normalization (),
    SpeakerArrangement::AmbisonicNormalization::Sn3d);
}

TEST_F (SpeakerArrangementTest, AmbisonicsCarriesOrderingAndNormalization)
{
  constexpr auto fuma = SpeakerArrangement::ambisonics (
    3, SpeakerArrangement::AmbisonicOrdering::FuMa,
    SpeakerArrangement::AmbisonicNormalization::MaxN);
  EXPECT_EQ (
    fuma.ambisonic_ordering (), SpeakerArrangement::AmbisonicOrdering::FuMa);
  EXPECT_EQ (
    fuma.ambisonic_normalization (),
    SpeakerArrangement::AmbisonicNormalization::MaxN);

  // conventions do not affect the channel count
  EXPECT_EQ (fuma.channel_count (), 16);
  EXPECT_EQ (
    fuma.channel_count (), SpeakerArrangement::ambisonics (3).channel_count ());
}

TEST_F (SpeakerArrangementTest, AmbisonicsWithDifferentConventionsAreNotEqual)
{
  EXPECT_NE (
    SpeakerArrangement::ambisonics (1),
    SpeakerArrangement::ambisonics (
      1, SpeakerArrangement::AmbisonicOrdering::FuMa,
      SpeakerArrangement::AmbisonicNormalization::MaxN));
}

TEST_F (SpeakerArrangementTest, ConventionsDoNotAffectNonAmbisonicEquality)
{
  // speaker and discrete arrangements leave the ambisonic fields at their
  // defaults, so equality stays driven by kind and payload alone
  EXPECT_EQ (SpeakerArrangement::stereo (), SpeakerArrangement::stereo ());
  EXPECT_EQ (
    SpeakerArrangement::discrete_channels (6),
    SpeakerArrangement::discrete_channels (6));
}

TEST_F (SpeakerArrangementTest, DiscreteChannels)
{
  constexpr auto discrete = SpeakerArrangement::discrete_channels (6);
  EXPECT_EQ (discrete.kind (), SpeakerArrangement::Kind::Discrete);
  EXPECT_EQ (discrete.channel_count (), 6);
  EXPECT_FALSE (discrete.is_mono ());
  EXPECT_FALSE (discrete.is_stereo ());
}

TEST_F (SpeakerArrangementTest, ChannelSpeakerOrder)
{
  // 5.1: channels follow increasing bit order (VST3/SMPTE convention)
  constexpr auto five_one = SpeakerArrangement::from_speaker_bits (speaker_bits (
    Speaker::Left, Speaker::Right, Speaker::Center, Speaker::Lfe,
    Speaker::LeftSurround, Speaker::RightSurround));
  EXPECT_EQ (five_one.channel_speaker (0), Speaker::Left);
  EXPECT_EQ (five_one.channel_speaker (1), Speaker::Right);
  EXPECT_EQ (five_one.channel_speaker (2), Speaker::Center);
  EXPECT_EQ (five_one.channel_speaker (3), Speaker::Lfe);
  EXPECT_EQ (five_one.channel_speaker (4), Speaker::LeftSurround);
  EXPECT_EQ (five_one.channel_speaker (5), Speaker::RightSurround);
  EXPECT_EQ (five_one.channel_speaker (6), std::nullopt);

  // non-speaker kinds have no channel speakers
  EXPECT_EQ (
    SpeakerArrangement::ambisonics (1).channel_speaker (0), std::nullopt);
  EXPECT_EQ (
    SpeakerArrangement::discrete_channels (2).channel_speaker (0), std::nullopt);
}

TEST_F (SpeakerArrangementTest, Equality)
{
  EXPECT_EQ (SpeakerArrangement::stereo (), SpeakerArrangement::stereo ());
  EXPECT_NE (SpeakerArrangement::stereo (), SpeakerArrangement::mono ());
  EXPECT_NE (
    SpeakerArrangement::discrete_channels (2), SpeakerArrangement::stereo ());
}

TEST_F (SpeakerArrangementTest, JsonRoundtrip)
{
  const auto arrangements = {
    SpeakerArrangement::mono (),
    SpeakerArrangement::stereo (),
    SpeakerArrangement::from_speaker_bits (speaker_bits (
      Speaker::Left, Speaker::Right, Speaker::Center, Speaker::Lfe,
      Speaker::LeftSurround, Speaker::RightSurround)),
    SpeakerArrangement::ambisonics (3),
    SpeakerArrangement::ambisonics (
      3, SpeakerArrangement::AmbisonicOrdering::FuMa,
      SpeakerArrangement::AmbisonicNormalization::MaxN),
    SpeakerArrangement::ambisonics (
      5, SpeakerArrangement::AmbisonicOrdering::Acn,
      SpeakerArrangement::AmbisonicNormalization::N3d),
    SpeakerArrangement::discrete_channels (12),
  };
  for (const auto &arrangement : arrangements)
    {
      nlohmann::json     j;
      SpeakerArrangement deserialized;
      to_json (j, arrangement);
      from_json (j, deserialized);
      EXPECT_EQ (deserialized, arrangement);
    }
}

TEST_F (SpeakerArrangementTest, JsonFormat)
{
  nlohmann::json j;
  to_json (j, SpeakerArrangement::stereo ());
  EXPECT_EQ (j.at ("kind").get<std::string> (), "speakers");
  EXPECT_EQ (
    j.at ("speakers").get<uint64_t> (),
    speaker_bits (Speaker::Left, Speaker::Right));

  nlohmann::json j_ambi;
  to_json (j_ambi, SpeakerArrangement::ambisonics (2));
  EXPECT_EQ (j_ambi.at ("kind").get<std::string> (), "ambisonics");
  EXPECT_EQ (j_ambi.at ("ambisonicOrder").get<uint64_t> (), 2);
  EXPECT_EQ (j_ambi.at ("ambisonicOrdering").get<std::string> (), "acn");
  EXPECT_EQ (j_ambi.at ("ambisonicNormalization").get<std::string> (), "sn3d");

  nlohmann::json j_discrete;
  to_json (j_discrete, SpeakerArrangement::discrete_channels (5));
  EXPECT_EQ (j_discrete.at ("kind").get<std::string> (), "discrete");
  EXPECT_EQ (j_discrete.at ("channels").get<uint64_t> (), 5);
}

TEST_F (SpeakerArrangementTest, JsonRejectsOutOfRangeAmbisonicOrder)
{
  const auto j = nlohmann::json{
    { "kind",                   "ambisonics" },
    { "ambisonicOrder",         20           },
    { "ambisonicOrdering",      "acn"        },
    { "ambisonicNormalization", "sn3d"       }
  };
  SpeakerArrangement arrangement;
  EXPECT_THROW (from_json (j, arrangement), std::out_of_range);
}

TEST_F (SpeakerArrangementTest, JsonRejectsFuMaOrderBeyondThird)
{
  const auto j = nlohmann::json{
    { "kind",                   "ambisonics" },
    { "ambisonicOrder",         5            },
    { "ambisonicOrdering",      "fuma"       },
    { "ambisonicNormalization", "maxn"       }
  };
  SpeakerArrangement arrangement;
  EXPECT_THROW (from_json (j, arrangement), std::out_of_range);
}

TEST_F (SpeakerArrangementTest, Formatting)
{
  EXPECT_EQ (fmt::format ("{}", SpeakerArrangement::mono ()), "Mono");
  EXPECT_EQ (fmt::format ("{}", SpeakerArrangement::stereo ()), "Stereo");
  EXPECT_EQ (
    fmt::format (
      "{}",
      SpeakerArrangement::from_speaker_bits (speaker_bits (
        Speaker::Left, Speaker::Right, Speaker::Center, Speaker::Lfe,
        Speaker::LeftSurround, Speaker::RightSurround))),
    "5.1");
  // the ambisonic conventions print via the global magic_enum formatter
  EXPECT_EQ (
    fmt::format ("{}", SpeakerArrangement::ambisonics (1)),
    "Ambisonics (order 1, Acn/Sn3d)");
  EXPECT_EQ (
    fmt::format (
      "{}",
      SpeakerArrangement::ambisonics (
        2, SpeakerArrangement::AmbisonicOrdering::FuMa,
        SpeakerArrangement::AmbisonicNormalization::MaxN)),
    "Ambisonics (order 2, FuMa/MaxN)");
  EXPECT_EQ (
    fmt::format ("{}", SpeakerArrangement::discrete_channels (6)),
    "Discrete (6 ch)");
}

} // namespace zrythm::dsp
