// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>
#include <type_traits>

#include "dsp/speaker_arrangement.h"

#include <fmt/format.h>
#include <magic_enum/magic_enum_format.hpp>
#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

NLOHMANN_JSON_SERIALIZE_ENUM (
  SpeakerArrangement::Kind,
  {
    { SpeakerArrangement::Kind::Speakers,   "speakers"   },
    { SpeakerArrangement::Kind::Ambisonics, "ambisonics" },
    { SpeakerArrangement::Kind::Discrete,   "discrete"   }
})

NLOHMANN_JSON_SERIALIZE_ENUM (
  SpeakerArrangement::AmbisonicOrdering,
  {
    { SpeakerArrangement::AmbisonicOrdering::Acn,  "acn"  },
    { SpeakerArrangement::AmbisonicOrdering::FuMa, "fuma" }
})

NLOHMANN_JSON_SERIALIZE_ENUM (
  SpeakerArrangement::AmbisonicNormalization,
  {
    { SpeakerArrangement::AmbisonicNormalization::Sn3d, "sn3d" },
    { SpeakerArrangement::AmbisonicNormalization::N3d,  "n3d"  },
    { SpeakerArrangement::AmbisonicNormalization::MaxN, "maxn" }
})

std::string
format_as (const SpeakerArrangement &arrangement)
{
  using enum SpeakerArrangement::Speaker;
  const auto bits = [] (auto &&... speakers) {
    return (std::to_underlying (speakers) | ...);
  };
  switch (arrangement.kind ())
    {
    case SpeakerArrangement::Kind::Speakers:
      {
        const auto speakers = arrangement.speaker_bits ();
        if (arrangement.is_mono ())
          return "Mono";
        if (arrangement.is_stereo ())
          return "Stereo";
        if (speakers == bits (Left, Right, LeftSurround, RightSurround))
          return "4.0 (Quad)";
        if (speakers == bits (Left, Right, Center, LeftSurround, RightSurround))
          return "5.0";
        if (
          speakers
          == bits (Left, Right, Center, Lfe, LeftSurround, RightSurround))
          return "5.1";
        if (
          speakers
          == bits (
            Left, Right, Center, Lfe, LeftSurround, RightSurround, SideLeft,
            SideRight))
          return "7.1";
        if (
          speakers
          == bits (
            Left, Right, Center, Lfe, LeftSurround, RightSurround, SideLeft,
            SideRight, TopFrontLeft, TopFrontRight, TopRearLeft, TopRearRight))
          return "7.1.4";
        return fmt::format (
          "Custom speakers ({} ch)", arrangement.channel_count ());
      }
    case SpeakerArrangement::Kind::Ambisonics:
      return fmt::format (
        "Ambisonics (order {}, {}/{})", arrangement.ambisonic_order (),
        arrangement.ambisonic_ordering (),
        arrangement.ambisonic_normalization ());
    case SpeakerArrangement::Kind::Discrete:
      return fmt::format ("Discrete ({} ch)", arrangement.channel_count ());
    }
  std::unreachable ();
}

static_assert (fmt::formattable<SpeakerArrangement>);

// arrangements are passed and stored by value throughout the graph, including
// on processing paths, so they must stay free of allocation and non-trivial
// copies
static_assert (std::is_trivially_copyable_v<SpeakerArrangement>);

void
to_json (nlohmann::json &j, const SpeakerArrangement &arrangement)
{
  j = nlohmann::json{
    { SpeakerArrangement::kKindKey, arrangement.kind_ }
  };
  switch (arrangement.kind_)
    {
    case SpeakerArrangement::Kind::Speakers:
      j[SpeakerArrangement::kSpeakersKey] = arrangement.payload_;
      break;
    case SpeakerArrangement::Kind::Ambisonics:
      j[SpeakerArrangement::kAmbisonicOrderKey] = arrangement.payload_;
      j[SpeakerArrangement::kAmbisonicOrderingKey] =
        arrangement.ambisonic_ordering_;
      j[SpeakerArrangement::kAmbisonicNormalizationKey] =
        arrangement.ambisonic_normalization_;
      break;
    case SpeakerArrangement::Kind::Discrete:
      j[SpeakerArrangement::kChannelsKey] = arrangement.payload_;
      break;
    }
}

void
from_json (const nlohmann::json &j, SpeakerArrangement &arrangement)
{
  j.at (SpeakerArrangement::kKindKey).get_to (arrangement.kind_);
  switch (arrangement.kind_)
    {
    case SpeakerArrangement::Kind::Speakers:
      j.at (SpeakerArrangement::kSpeakersKey).get_to (arrangement.payload_);
      break;
    case SpeakerArrangement::Kind::Ambisonics:
      j.at (SpeakerArrangement::kAmbisonicOrderKey).get_to (arrangement.payload_);
      j.at (SpeakerArrangement::kAmbisonicOrderingKey)
        .get_to (arrangement.ambisonic_ordering_);
      j.at (SpeakerArrangement::kAmbisonicNormalizationKey)
        .get_to (arrangement.ambisonic_normalization_);
      // orders above the maximum overflow the channel count, so reject them at
      // the deserialization boundary
      if (arrangement.payload_ > SpeakerArrangement::kMaxAmbisonicOrder)
        {
          throw std::out_of_range (
            fmt::format (
              "Ambisonic order {} is out of range (max {})",
              arrangement.payload_, SpeakerArrangement::kMaxAmbisonicOrder));
        }
      if (
        arrangement.ambisonic_ordering_
          == SpeakerArrangement::AmbisonicOrdering::FuMa
        && arrangement.payload_ > SpeakerArrangement::kMaxFuMaAmbisonicOrder)
        {
          throw std::out_of_range (
            fmt::format (
              "FuMa ordering is only defined up to order {}, got {}",
              SpeakerArrangement::kMaxFuMaAmbisonicOrder, arrangement.payload_));
        }
      break;
    case SpeakerArrangement::Kind::Discrete:
      j.at (SpeakerArrangement::kChannelsKey).get_to (arrangement.payload_);
      break;
    }
}

} // namespace zrythm::dsp
