// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>

#include "plugins/clap_speaker_arrangement.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

using dsp::SpeakerArrangement;

using clap_speaker_arrangement::ambisonic_config_from_arrangement;
using clap_speaker_arrangement::arrangement_from_ambisonic_config;
using clap_speaker_arrangement::arrangement_from_surround_channel_map;
using clap_speaker_arrangement::port_type_from_arrangement;
using clap_speaker_arrangement::speaker_from_surround_id;
using clap_speaker_arrangement::surround_channel_map_from_arrangement;
using clap_speaker_arrangement::surround_channel_permutation;
using clap_speaker_arrangement::surround_id_from_speaker;

class ClapSpeakerArrangementTest : public ::testing::Test
{
protected:
  // canonical 5.1 map: FL FR FC LFE BL BR
  static constexpr std::array<uint8_t, 6> kMap5Point1 = {
    CLAP_SURROUND_FL,  CLAP_SURROUND_FR, CLAP_SURROUND_FC,
    CLAP_SURROUND_LFE, CLAP_SURROUND_BL, CLAP_SURROUND_BR
  };
};

TEST_F (ClapSpeakerArrangementTest, SurroundMapConvertsToSpeakers)
{
  const auto arrangement = arrangement_from_surround_channel_map (kMap5Point1);
  ASSERT_TRUE (arrangement.has_value ());
  EXPECT_EQ (arrangement->kind (), SpeakerArrangement::Kind::Speakers);
  EXPECT_EQ (arrangement->channel_count (), 6);
  EXPECT_TRUE (
    arrangement->has_speaker (SpeakerArrangement::Speaker::LeftSurround));
  EXPECT_TRUE (arrangement->has_speaker (SpeakerArrangement::Speaker::Lfe));

  // canonical order: increasing bit position
  EXPECT_EQ (
    arrangement->channel_speaker (0), SpeakerArrangement::Speaker::Left);
  EXPECT_EQ (arrangement->channel_speaker (3), SpeakerArrangement::Speaker::Lfe);
}

TEST_F (ClapSpeakerArrangementTest, NonCanonicalMapKeepsArrangementAndPermutes)
{
  // same speaker set as 5.1, but the plugin puts the center channel first
  const std::array<uint8_t, 6> map = {
    CLAP_SURROUND_FC,  CLAP_SURROUND_FL, CLAP_SURROUND_FR,
    CLAP_SURROUND_LFE, CLAP_SURROUND_BL, CLAP_SURROUND_BR
  };
  const auto arrangement = arrangement_from_surround_channel_map (map);
  ASSERT_TRUE (arrangement.has_value ());
  EXPECT_EQ (*arrangement, *arrangement_from_surround_channel_map (kMap5Point1));

  const auto permutation = surround_channel_permutation (map);
  ASSERT_TRUE (permutation.has_value ());
  // plugin channel 0 (center) is canonical channel 2, plugin channels 1/2
  // (left/right) are canonical channels 0/1, the rest is untouched
  const std::vector<uint8_t> expected = { 2, 0, 1, 3, 4, 5 };
  EXPECT_EQ (*permutation, expected);

  // an already-canonical map gets the identity permutation
  const auto identity = surround_channel_permutation (kMap5Point1);
  ASSERT_TRUE (identity.has_value ());
  EXPECT_EQ (*identity, (std::vector<uint8_t>{ 0, 1, 2, 3, 4, 5 }));
}

TEST_F (ClapSpeakerArrangementTest, InvalidMapsDoNotConvert)
{
  // unknown id (beyond CLAP_SURROUND_TSR)
  const std::array<uint8_t, 3> unknown = {
    CLAP_SURROUND_FL, CLAP_SURROUND_FR, 42
  };
  EXPECT_FALSE (arrangement_from_surround_channel_map (unknown).has_value ());
  EXPECT_FALSE (surround_channel_permutation (unknown).has_value ());

  // speaker named twice
  const std::array<uint8_t, 2> duplicate = {
    CLAP_SURROUND_FL, CLAP_SURROUND_FL
  };
  EXPECT_FALSE (arrangement_from_surround_channel_map (duplicate).has_value ());

  // empty map
  EXPECT_FALSE (arrangement_from_surround_channel_map ({}).has_value ());
}

TEST_F (ClapSpeakerArrangementTest, AmbisonicConfigConvertsByTriple)
{
  const auto first_order = arrangement_from_ambisonic_config (
    4, CLAP_AMBISONIC_ORDERING_ACN, CLAP_AMBISONIC_NORMALIZATION_SN3D);
  ASSERT_TRUE (first_order.has_value ());
  EXPECT_EQ (
    *first_order,
    SpeakerArrangement::ambisonics (
      1, SpeakerArrangement::AmbisonicOrdering::Acn,
      SpeakerArrangement::AmbisonicNormalization::Sn3d));

  const auto fuma_second_order = arrangement_from_ambisonic_config (
    9, CLAP_AMBISONIC_ORDERING_FUMA, CLAP_AMBISONIC_NORMALIZATION_MAXN);
  ASSERT_TRUE (fuma_second_order.has_value ());
  EXPECT_EQ (
    *fuma_second_order,
    SpeakerArrangement::ambisonics (
      2, SpeakerArrangement::AmbisonicOrdering::FuMa,
      SpeakerArrangement::AmbisonicNormalization::MaxN));

  // round trip
  const auto round_trip = ambisonic_config_from_arrangement (*fuma_second_order);
  ASSERT_TRUE (round_trip.has_value ());
  EXPECT_EQ (round_trip->first, CLAP_AMBISONIC_ORDERING_FUMA);
  EXPECT_EQ (round_trip->second, CLAP_AMBISONIC_NORMALIZATION_MAXN);
}

TEST_F (ClapSpeakerArrangementTest, AmbisonicConfigRejectsUnrepresentable)
{
  // FuMa is only defined up to 3rd order (25 channels is 4th order)
  EXPECT_FALSE (
    arrangement_from_ambisonic_config (
      25, CLAP_AMBISONIC_ORDERING_FUMA, CLAP_AMBISONIC_NORMALIZATION_MAXN)
      .has_value ());

  // horizontal-only schemes carry 2 * order + 1 channels, which the dsp
  // triple cannot describe
  EXPECT_FALSE (
    arrangement_from_ambisonic_config (
      3, CLAP_AMBISONIC_ORDERING_ACN, CLAP_AMBISONIC_NORMALIZATION_SN2D)
      .has_value ());

  // 5 channels is not (order + 1)^2
  EXPECT_FALSE (
    arrangement_from_ambisonic_config (
      5, CLAP_AMBISONIC_ORDERING_ACN, CLAP_AMBISONIC_NORMALIZATION_SN3D)
      .has_value ());

  // unknown constants
  EXPECT_FALSE (
    arrangement_from_ambisonic_config (4, 42, CLAP_AMBISONIC_NORMALIZATION_SN3D)
      .has_value ());
  EXPECT_FALSE (
    arrangement_from_ambisonic_config (4, CLAP_AMBISONIC_ORDERING_ACN, 42)
      .has_value ());
}

TEST_F (ClapSpeakerArrangementTest, PortTypeForArrangement)
{
  EXPECT_STREQ (
    port_type_from_arrangement (SpeakerArrangement::mono ()), CLAP_PORT_MONO);
  EXPECT_STREQ (
    port_type_from_arrangement (SpeakerArrangement::stereo ()),
    CLAP_PORT_STEREO);
  EXPECT_STREQ (
    port_type_from_arrangement (
      *arrangement_from_surround_channel_map (kMap5Point1)),
    CLAP_PORT_SURROUND);
  EXPECT_STREQ (
    port_type_from_arrangement (SpeakerArrangement::ambisonics (1)),
    CLAP_PORT_AMBISONIC);
  EXPECT_EQ (
    port_type_from_arrangement (SpeakerArrangement::discrete_channels (8)),
    nullptr);
}

TEST_F (ClapSpeakerArrangementTest, ChannelMapRoundTripsInCanonicalOrder)
{
  const auto arrangement = arrangement_from_surround_channel_map (kMap5Point1);
  ASSERT_TRUE (arrangement.has_value ());

  const auto map = surround_channel_map_from_arrangement (*arrangement);
  ASSERT_TRUE (map.has_value ());
  EXPECT_EQ (
    *map, (std::vector<uint8_t> (kMap5Point1.begin (), kMap5Point1.end ())));

  // round trip
  EXPECT_EQ (arrangement_from_surround_channel_map (*map), arrangement);

  // mono has no CLAP surround speaker for its Mono bit (mono ports use
  // CLAP_PORT_MONO with discarded details instead)
  EXPECT_FALSE (
    surround_channel_map_from_arrangement (SpeakerArrangement::mono ())
      .has_value ());

  // non-speaker kinds have no channel map
  EXPECT_FALSE (
    surround_channel_map_from_arrangement (SpeakerArrangement::ambisonics (1))
      .has_value ());
  EXPECT_FALSE (
    surround_channel_map_from_arrangement (
      SpeakerArrangement::discrete_channels (8))
      .has_value ());
}

// Every CLAP surround id maps to a speaker and back — including TSL/TSR,
// whose speaker bits (24/25) break the id == bit position pattern
TEST_F (ClapSpeakerArrangementTest, EverySurroundIdRoundTrips)
{
  constexpr std::array<uint8_t, 20> ids{
    CLAP_SURROUND_FL,  CLAP_SURROUND_FR,  CLAP_SURROUND_FC,  CLAP_SURROUND_LFE,
    CLAP_SURROUND_BL,  CLAP_SURROUND_BR,  CLAP_SURROUND_FLC, CLAP_SURROUND_FRC,
    CLAP_SURROUND_BC,  CLAP_SURROUND_SL,  CLAP_SURROUND_SR,  CLAP_SURROUND_TC,
    CLAP_SURROUND_TFL, CLAP_SURROUND_TFC, CLAP_SURROUND_TFR, CLAP_SURROUND_TBL,
    CLAP_SURROUND_TBC, CLAP_SURROUND_TBR, CLAP_SURROUND_TSL, CLAP_SURROUND_TSR,
  };
  for (const uint8_t id : ids)
    {
      const auto speaker = speaker_from_surround_id (id);
      ASSERT_TRUE (speaker.has_value ()) << "id " << static_cast<int> (id);
      EXPECT_EQ (surround_id_from_speaker (*speaker), id);
    }
}

} // namespace zrythm::plugins
