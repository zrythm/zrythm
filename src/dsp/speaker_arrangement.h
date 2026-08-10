// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * The @ref Speaker bit values match the kSpeaker* constants from the VST3 SDK
 * (pluginterfaces/vst/vstspeaker.h, MIT, © Steinberg Media Technologies GmbH).
 */

#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json_fwd.hpp>

using namespace std::string_view_literals;

namespace zrythm::dsp
{

/**
 * @brief Description of what the channels of an audio bus represent.
 *
 * Three kinds:
 * - @ref Kind::Speakers: a bitmask of speaker positions (see @ref Speaker).
 *   Channel N corresponds to the N-th set bit in increasing bit order (the
 *   VST3/SMPTE channel ordering convention).
 * - @ref Kind::Ambisonics: full-sphere ambisonics of a given order, with
 *   (order + 1)^2 channels, carrying its own channel ordering and
 *   normalization conventions (see @ref AmbisonicOrdering and
 *   @ref AmbisonicNormalization).
 * - @ref Kind::Discrete: N channels with unspecified semantics (used when a
 *   plugin reports channels without layout information).
 *
 * Speaker bit values intentionally match the VST3 SDK's
 * @c SpeakerArrangement constants (pluginterfaces/vst/vstspeaker.h) so that
 * VST3 conversion is lossless, and because that set covers every CLAP
 * surround speaker and every LV2 port-groups discrete channel designation,
 * as well as all ITU-R BS.2051 configurations up to 22.2.
 */
class SpeakerArrangement
{
public:
  enum class Kind : uint8_t
  {
    Speakers,
    Ambisonics,
    Discrete,
  };

  /**
   * @brief Speaker positions as a bitmask.
   *
   * Values match VST3's @c kSpeaker* constants. The ambisonic pseudo-speakers
   * (VST3 @c kSpeakerACN*) are intentionally absent: ambisonics is expressed
   * via @ref Kind::Ambisonics instead.
   */
  enum class Speaker : uint64_t
  {
    Left = 1ULL << 0,
    Right = 1ULL << 1,
    Center = 1ULL << 2,
    Lfe = 1ULL << 3,
    LeftSurround = 1ULL << 4,
    RightSurround = 1ULL << 5,
    LeftOfCenter = 1ULL << 6,
    RightOfCenter = 1ULL << 7,
    SurroundCenter = 1ULL << 8,
    SideLeft = 1ULL << 9,
    SideRight = 1ULL << 10,
    TopCenter = 1ULL << 11,
    TopFrontLeft = 1ULL << 12,
    TopFrontCenter = 1ULL << 13,
    TopFrontRight = 1ULL << 14,
    TopRearLeft = 1ULL << 15,
    TopRearCenter = 1ULL << 16,
    TopRearRight = 1ULL << 17,
    Lfe2 = 1ULL << 18,
    Mono = 1ULL << 19,
    TopSideLeft = 1ULL << 24,
    TopSideRight = 1ULL << 25,
    LeftOfCenterSurround = 1ULL << 26,
    RightOfCenterSurround = 1ULL << 27,
    BottomFrontLeft = 1ULL << 28,
    BottomFrontCenter = 1ULL << 29,
    BottomFrontRight = 1ULL << 30,
    ProximityLeft = 1ULL << 31,
    ProximityRight = 1ULL << 32,
    BottomSideLeft = 1ULL << 33,
    BottomSideRight = 1ULL << 34,
    BottomRearLeft = 1ULL << 35,
    BottomRearCenter = 1ULL << 36,
    BottomRearRight = 1ULL << 37,
    LeftWide = 1ULL << 59,
    RightWide = 1ULL << 60,
  };

  /**
   * @brief Channel ordering convention for @ref Kind::Ambisonics.
   */
  enum class AmbisonicOrdering : uint8_t
  {
    /** Ambisonic Channel Number, the convention used by AmbiX. */
    Acn,

    /** Furse-Malham, defined up to @ref kMaxFuMaAmbisonicOrder only. */
    FuMa,
  };

  /**
   * @brief Normalization convention for @ref Kind::Ambisonics.
   *
   * Only full-sphere schemes are modelled, which is what keeps the channel
   * count at (order + 1)^2. The horizontal-only schemes (SN2D/N2D) carry
   * 2 * order + 1 channels and would need their own kind.
   */
  enum class AmbisonicNormalization : uint8_t
  {
    /** Schmidt semi-normalized, the convention used by AmbiX. */
    Sn3d,

    /** Fully normalized (orthonormal). */
    N3d,

    /** Maximum value normalization, the convention used by FuMa. */
    MaxN,
  };

  static constexpr uint8_t kMaxAmbisonicOrder = 7;

  /** Highest order the FuMa ordering is defined for. */
  static constexpr uint8_t kMaxFuMaAmbisonicOrder = 3;

  /**
   * @brief Default: an empty speaker set.
   */
  constexpr SpeakerArrangement () = default;

  static constexpr SpeakerArrangement from_speaker_bits (uint64_t bits)
  {
    return { Kind::Speakers, bits };
  }
  static constexpr SpeakerArrangement mono ()
  {
    return from_speaker_bits (std::to_underlying (Speaker::Mono));
  }
  static constexpr SpeakerArrangement stereo ()
  {
    return from_speaker_bits (
      std::to_underlying (Speaker::Left) | std::to_underlying (Speaker::Right));
  }
  /**
   * @brief Full-sphere ambisonics of the given order.
   *
   * Defaults to AmbiX, i.e. ACN ordering with SN3D normalization.
   *
   * @param order Ambisonic order, at most @ref kMaxAmbisonicOrder, or at most
   * @ref kMaxFuMaAmbisonicOrder when @p ordering is AmbisonicOrdering::FuMa.
   */
  static constexpr SpeakerArrangement ambisonics (
    uint8_t                order,
    AmbisonicOrdering      ordering = AmbisonicOrdering::Acn,
    AmbisonicNormalization normalization = AmbisonicNormalization::Sn3d)
  {
    assert (order <= kMaxAmbisonicOrder);
    assert (
      ordering != AmbisonicOrdering::FuMa || order <= kMaxFuMaAmbisonicOrder);
    auto arrangement = SpeakerArrangement{ Kind::Ambisonics, order };
    arrangement.ambisonic_ordering_ = ordering;
    arrangement.ambisonic_normalization_ = normalization;
    return arrangement;
  }
  static constexpr SpeakerArrangement discrete_channels (uint8_t channels)
  {
    return { Kind::Discrete, channels };
  }

  [[nodiscard]] constexpr Kind kind () const { return kind_; }

  [[nodiscard]] constexpr uint8_t channel_count () const
  {
    switch (kind_)
      {
      case Kind::Speakers:
        return static_cast<uint8_t> (std::popcount (payload_));
      case Kind::Ambisonics:
        {
          const auto channels_per_dimension = payload_ + 1;
          return static_cast<uint8_t> (
            channels_per_dimension * channels_per_dimension);
        }
      case Kind::Discrete:
        return static_cast<uint8_t> (payload_);
      }
    std::unreachable ();
  }

  [[nodiscard]] constexpr bool is_mono () const
  {
    return kind_ == Kind::Speakers
           && payload_ == std::to_underlying (Speaker::Mono);
  }
  [[nodiscard]] constexpr bool is_stereo () const
  {
    return kind_ == Kind::Speakers && payload_ == stereo ().payload_;
  }

  /**
   * @brief Speaker bitmask. Only valid when kind() is Kind::Speakers.
   */
  [[nodiscard]] constexpr uint64_t speaker_bits () const
  {
    assert (kind_ == Kind::Speakers);
    return payload_;
  }

  /**
   * @brief Ambisonic order. Only valid when kind() is Kind::Ambisonics.
   */
  [[nodiscard]] constexpr uint8_t ambisonic_order () const
  {
    assert (kind_ == Kind::Ambisonics);
    return static_cast<uint8_t> (payload_);
  }

  /**
   * @brief Ambisonic channel ordering. Only valid when kind() is
   * Kind::Ambisonics.
   */
  [[nodiscard]] constexpr AmbisonicOrdering ambisonic_ordering () const
  {
    assert (kind_ == Kind::Ambisonics);
    return ambisonic_ordering_;
  }

  /**
   * @brief Ambisonic normalization. Only valid when kind() is
   * Kind::Ambisonics.
   */
  [[nodiscard]] constexpr AmbisonicNormalization
  ambisonic_normalization () const
  {
    assert (kind_ == Kind::Ambisonics);
    return ambisonic_normalization_;
  }

  [[nodiscard]] constexpr bool has_speaker (Speaker speaker) const
  {
    return kind_ == Kind::Speakers
           && (payload_ & std::to_underlying (speaker)) != 0;
  }

  /**
   * @brief The speaker carried by the given channel index.
   *
   * Channels are ordered by increasing bit position (VST3/SMPTE convention).
   *
   * @return The speaker, or std::nullopt if the kind is not Kind::Speakers or
   * the index is out of range.
   */
  [[nodiscard]] constexpr std::optional<Speaker>
  channel_speaker (uint8_t channel_index) const
  {
    if (kind_ != Kind::Speakers || channel_index >= channel_count ())
      return std::nullopt;
    auto bits = payload_;
    for (uint8_t i = 0; i < channel_index; ++i)
      bits &= bits - 1; // clear lowest set bit
    return static_cast<Speaker> (bits & (~bits + 1));
  }

  constexpr bool operator== (const SpeakerArrangement &) const = default;

  friend void
  to_json (nlohmann::json &j, const SpeakerArrangement &arrangement);
  friend void
  from_json (const nlohmann::json &j, SpeakerArrangement &arrangement);

private:
  constexpr SpeakerArrangement (Kind kind, uint64_t payload)
      : kind_ (kind), payload_ (payload)
  {
  }

  static constexpr auto kKindKey = "kind"sv;
  static constexpr auto kSpeakersKey = "speakers"sv;
  static constexpr auto kAmbisonicOrderKey = "ambisonicOrder"sv;
  static constexpr auto kAmbisonicOrderingKey = "ambisonicOrdering"sv;
  static constexpr auto kAmbisonicNormalizationKey = "ambisonicNormalization"sv;
  static constexpr auto kChannelsKey = "channels"sv;

  Kind kind_{};

  /**
   * @brief Ambisonic conventions, occupying padding that @ref kind_ leaves
   * behind. Only meaningful when @ref kind_ is Kind::Ambisonics, and left at
   * their defaults otherwise so that equality stays kind-driven.
   */
  AmbisonicOrdering      ambisonic_ordering_{};
  AmbisonicNormalization ambisonic_normalization_{};

  /**
   * @brief Payload interpreted per @ref kind_: speaker bitmask
   * (Kind::Speakers), ambisonic order (Kind::Ambisonics) or channel count
   * (Kind::Discrete).
   */
  uint64_t payload_{};
};

/**
 * @brief fmt printing support (for logging).
 */
[[nodiscard]] std::string
format_as (const SpeakerArrangement &arrangement);

} // namespace zrythm::dsp
