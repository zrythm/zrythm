// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/vst3_speaker_arrangement.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

using dsp::SpeakerArrangement;

using vst3_speaker_arrangement::from_dsp;
using vst3_speaker_arrangement::to_dsp;
namespace Vst = Steinberg::Vst;

class Vst3SpeakerArrangementTest : public ::testing::Test
{
protected:
  static constexpr Vst::SpeakerArrangement kArr5Point1 =
    Vst::kSpeakerL | Vst::kSpeakerR | Vst::kSpeakerC | Vst::kSpeakerLfe
    | Vst::kSpeakerLs | Vst::kSpeakerRs;
};

TEST_F (Vst3SpeakerArrangementTest, SpeakerLayoutsConvertLosslessly)
{
  const auto stereo = to_dsp (Vst::SpeakerArr::kStereo, 2);
  EXPECT_EQ (stereo, SpeakerArrangement::stereo ());

  const auto mono = to_dsp (Vst::SpeakerArr::kMono, 1);
  EXPECT_EQ (mono, SpeakerArrangement::mono ());

  const auto surround = to_dsp (kArr5Point1, 6);
  EXPECT_EQ (surround.kind (), SpeakerArrangement::Kind::Speakers);
  EXPECT_EQ (surround.channel_count (), 6);
  EXPECT_TRUE (surround.has_speaker (SpeakerArrangement::Speaker::LeftSurround));
  EXPECT_TRUE (surround.has_speaker (SpeakerArrangement::Speaker::Lfe));

  // round trip
  EXPECT_EQ (from_dsp (surround), kArr5Point1);
  EXPECT_EQ (from_dsp (stereo), Vst::SpeakerArr::kStereo);
}

TEST_F (Vst3SpeakerArrangementTest, EmptyArrangementFallsBackToChannelCount)
{
  EXPECT_EQ (to_dsp (0, 1), SpeakerArrangement::mono ());
  EXPECT_EQ (to_dsp (0, 2), SpeakerArrangement::stereo ());
  const auto discrete = to_dsp (0, 6);
  EXPECT_EQ (discrete.kind (), SpeakerArrangement::Kind::Discrete);
  EXPECT_EQ (discrete.channel_count (), 6);
}

TEST_F (Vst3SpeakerArrangementTest, AmbisonicsConvertByOrder)
{
  const auto first_order = to_dsp (Vst::SpeakerArr::kAmbi1stOrderACN, 4);
  EXPECT_EQ (first_order.kind (), SpeakerArrangement::Kind::Ambisonics);
  EXPECT_EQ (first_order.ambisonic_order (), 1);
  EXPECT_EQ (first_order.channel_count (), 4);

  const auto third_order = to_dsp (Vst::SpeakerArr::kAmbi3rdOrderACN, 16);
  EXPECT_EQ (third_order.kind (), SpeakerArrangement::Kind::Ambisonics);
  EXPECT_EQ (third_order.ambisonic_order (), 3);

  EXPECT_EQ (from_dsp (first_order), Vst::SpeakerArr::kAmbi1stOrderACN);
  EXPECT_EQ (from_dsp (third_order), Vst::SpeakerArr::kAmbi3rdOrderACN);
}

TEST_F (Vst3SpeakerArrangementTest, AmbisonicsAboveVst3OrderCannotConvertBack)
{
  const auto fifth_order = SpeakerArrangement::ambisonics (5);
  EXPECT_EQ (from_dsp (fifth_order), 0);
}

TEST_F (Vst3SpeakerArrangementTest, NonContiguousAcnBitsBecomeDiscrete)
{
  // ACN0 and ACN1 only: a valid 2-channel ambisonic set would be impossible
  // (2 is not (order + 1)^2)
  const auto weird = to_dsp (Vst::kSpeakerACN0 | Vst::kSpeakerACN1, 2);
  EXPECT_EQ (weird.kind (), SpeakerArrangement::Kind::Discrete);
  EXPECT_EQ (weird.channel_count (), 2);
}

TEST_F (Vst3SpeakerArrangementTest, UnknownBitsBecomeDiscrete)
{
  // bit 61 is not a defined VST3 speaker: a 6-channel bus with an unknown
  // bit falls back to channel-count semantics
  const auto unknown =
    to_dsp (kArr5Point1 | (Vst::SpeakerArrangement{ 1 } << 61), 6);
  EXPECT_EQ (unknown.kind (), SpeakerArrangement::Kind::Discrete);
  EXPECT_EQ (unknown.channel_count (), 6);

  // ...where the count fallback is mono/stereo, those still apply
  EXPECT_EQ (
    to_dsp (Vst::SpeakerArrangement{ 1 } << 61 | Vst::SpeakerArrangement{ 1 }, 1),
    SpeakerArrangement::mono ());
}

TEST_F (Vst3SpeakerArrangementTest, ChannelCountMismatchBecomesDiscrete)
{
  // stereo arrangement reported for a 6-channel bus: no honest mapping
  const auto mismatch = to_dsp (Vst::SpeakerArr::kStereo, 6);
  EXPECT_EQ (mismatch.kind (), SpeakerArrangement::Kind::Discrete);
  EXPECT_EQ (mismatch.channel_count (), 6);
}

TEST_F (Vst3SpeakerArrangementTest, DiscreteCannotConvertBack)
{
  EXPECT_EQ (from_dsp (SpeakerArrangement::discrete_channels (8)), 0);
}

} // namespace zrythm::plugins
