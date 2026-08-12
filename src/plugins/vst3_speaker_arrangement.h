// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <ranges>

#include "dsp/speaker_arrangement.h"

#include <pluginterfaces/vst/ivstaudioprocessor.h>

namespace zrythm::plugins
{

/**
 * @brief Conversions between VST3 speaker arrangements and the SDK-free
 * dsp::SpeakerArrangement.
 *
 * dsp speaker bit values intentionally match VST3's kSpeaker* constants, so
 * speaker layouts convert losslessly. VST3 expresses ambisonics as ACN
 * pseudo-speaker bits (ACN0-ACN3 at bits 20-23, ACN4-ACN24 at bits 38-58);
 * the dsp type keeps ambisonics as a separate kind, so the conversion
 * detects contiguous ACN sets. VST3 carries no ordering/normalization
 * metadata, so ambisonics maps to the AmbiX convention (ACN ordering, SN3D
 * normalization), and dsp ambisonics with other conventions still convert to
 * the same VST3 arrangement (the convention difference is dropped at the
 * boundary).
 */
namespace vst3_speaker_arrangement
{

/** Bitmask of the VST3 ACN pseudo-speaker bits. */
constexpr Steinberg::uint64 kAcnBits =
  (Steinberg::uint64{ 0xF } << 20)
  | (((Steinberg::uint64{ 1 } << 21) - 1) << 38);

/** Bitmask of every non-ACN speaker bit VST3 defines (and the dsp type
 * supports). */
constexpr Steinberg::uint64 kKnownSpeakerBits =
  ((Steinberg::uint64{ 1 } << 20) - 1)           // bits 0-19
  | (((Steinberg::uint64{ 1 } << 14) - 1) << 24) // bits 24-37
  | (Steinberg::uint64{ 1 } << 59) | (Steinberg::uint64{ 1 } << 60);

/** Highest ambisonic order VST3 can express (ACN0-ACN24). */
constexpr uint8_t kMaxVst3AmbisonicOrder = 4;

/**
 * @brief Returns the bitmask of ACN0 through ACN(channel_count - 1).
 */
constexpr Steinberg::uint64
acn_mask_for_channel_count (Steinberg::int32 channel_count)
{
  Steinberg::uint64 mask = 0;
  for (const auto i : std::views::iota (0, channel_count))
    mask |= Steinberg::uint64{ 1 } << (i < 4 ? 20 + i : 38 + (i - 4));
  return mask;
}

/**
 * @brief Converts a live VST3 bus description to a speaker arrangement.
 *
 * Falls back to channel-count semantics (mono / stereo / discrete) whenever
 * the reported arrangement cannot honestly describe the bus: no arrangement
 * reported (0), ambisonic bits that don't form a contiguous ACN set,
 * unknown bits, or a speaker count that disagrees with the bus channel
 * count.
 *
 * @param arrangement The arrangement from IAudioProcessor::getBusArrangement
 * (0 when the query failed).
 * @param channel_count The bus channel count from IComponent::getBusInfo.
 */
constexpr dsp::SpeakerArrangement
to_dsp (
  Steinberg::Vst::SpeakerArrangement arrangement,
  Steinberg::int32                   channel_count)
{
  assert (channel_count >= 1);

  const auto count_fallback = [channel_count] {
    const auto count = static_cast<uint8_t> (channel_count);
    return count == 1 ? dsp::SpeakerArrangement::mono ()
           : count == 2
             ? dsp::SpeakerArrangement::stereo ()
             : dsp::SpeakerArrangement::discrete_channels (count);
  };

  const auto bits = static_cast<Steinberg::uint64> (arrangement);
  if (bits == 0)
    return count_fallback ();

  if ((bits & kAcnBits) != 0)
    {
      for (
        const auto order :
        std::views::iota (1, dsp::SpeakerArrangement::kMaxAmbisonicOrder + 1))
        {
          const auto channels =
            static_cast<Steinberg::int32> ((order + 1) * (order + 1));
          if (
            channel_count == channels
            && bits == acn_mask_for_channel_count (channels))
            {
              return dsp::SpeakerArrangement::ambisonics (
                static_cast<uint8_t> (order));
            }
        }
      return dsp::SpeakerArrangement::discrete_channels (
        static_cast<uint8_t> (channel_count));
    }

  if ((bits & ~kKnownSpeakerBits) != 0 || std::popcount (bits) != channel_count)
    {
      return count_fallback ();
    }
  return dsp::SpeakerArrangement::from_speaker_bits (bits);
}

/**
 * @brief Converts a speaker arrangement to a VST3 arrangement for
 * IAudioProcessor::setBusArrangements.
 *
 * @return The VST3 arrangement, or 0 when the arrangement has no VST3
 * representation (Kind::Discrete, or ambisonics above
 * @ref kMaxVst3AmbisonicOrder) — the negotiation will not apply that bus.
 */
constexpr Steinberg::Vst::SpeakerArrangement
from_dsp (const dsp::SpeakerArrangement &arrangement)
{
  switch (arrangement.kind ())
    {
    case dsp::SpeakerArrangement::Kind::Speakers:
      return static_cast<Steinberg::Vst::SpeakerArrangement> (
        arrangement.speaker_bits ());
    case dsp::SpeakerArrangement::Kind::Ambisonics:
      {
        const auto order = arrangement.ambisonic_order ();
        if (order > kMaxVst3AmbisonicOrder)
          return 0;
        return static_cast<Steinberg::Vst::SpeakerArrangement> (
          acn_mask_for_channel_count (arrangement.channel_count ()));
      }
    case dsp::SpeakerArrangement::Kind::Discrete:
      return 0;
    }
  return 0;
}

} // namespace vst3_speaker_arrangement

} // namespace zrythm::plugins
