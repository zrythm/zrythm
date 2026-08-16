// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "dsp/speaker_arrangement.h"

#include <clap/ext/ambisonic.h>
#include <clap/ext/surround.h>

namespace zrythm::plugins
{

/**
 * @brief Conversions between CLAP audio port descriptions and the SDK-free
 * dsp::SpeakerArrangement.
 *
 * CLAP describes layouts with a port type string (mono / stereo / surround /
 * ambisonic) plus a per-channel speaker map from the surround extension, or
 * an ordering/normalization pair from the ambisonic extension. The dsp
 * speaker set covers every CLAP surround speaker, so surround layouts convert
 * losslessly. The surround channel map additionally defines the plugin's wire
 * order, which surround_channel_permutation() exposes so the host can order
 * buffer pointers without copying samples (ambisonic ports carry their
 * ordering inside the arrangement and never need a permutation).
 */
namespace clap_speaker_arrangement
{

using dsp::SpeakerArrangement;

/**
 * @brief The dsp speaker for a CLAP surround channel id (CLAP_SURROUND_*).
 *
 * @return std::nullopt when the id is not a defined CLAP surround speaker.
 */
[[nodiscard]]
constexpr std::optional<SpeakerArrangement::Speaker>
speaker_from_surround_id (uint8_t id)
{
  using Speaker = SpeakerArrangement::Speaker;
  switch (id)
    {
    case CLAP_SURROUND_FL:
      return Speaker::Left;
    case CLAP_SURROUND_FR:
      return Speaker::Right;
    case CLAP_SURROUND_FC:
      return Speaker::Center;
    case CLAP_SURROUND_LFE:
      return Speaker::Lfe;
    case CLAP_SURROUND_BL:
      return Speaker::LeftSurround;
    case CLAP_SURROUND_BR:
      return Speaker::RightSurround;
    case CLAP_SURROUND_FLC:
      return Speaker::LeftOfCenter;
    case CLAP_SURROUND_FRC:
      return Speaker::RightOfCenter;
    case CLAP_SURROUND_BC:
      return Speaker::SurroundCenter;
    case CLAP_SURROUND_SL:
      return Speaker::SideLeft;
    case CLAP_SURROUND_SR:
      return Speaker::SideRight;
    case CLAP_SURROUND_TC:
      return Speaker::TopCenter;
    case CLAP_SURROUND_TFL:
      return Speaker::TopFrontLeft;
    case CLAP_SURROUND_TFC:
      return Speaker::TopFrontCenter;
    case CLAP_SURROUND_TFR:
      return Speaker::TopFrontRight;
    case CLAP_SURROUND_TBL:
      return Speaker::TopRearLeft;
    case CLAP_SURROUND_TBC:
      return Speaker::TopRearCenter;
    case CLAP_SURROUND_TBR:
      return Speaker::TopRearRight;
    case CLAP_SURROUND_TSL:
      return Speaker::TopSideLeft;
    case CLAP_SURROUND_TSR:
      return Speaker::TopSideRight;
    default:
      return std::nullopt;
    }
}

/**
 * @brief The CLAP surround channel id for a dsp speaker, for
 * clap_audio_port_configuration_request channel maps.
 *
 * @return std::nullopt when the speaker has no CLAP surround id (Mono, Lfe2,
 * LeftOfCenterSurround, RightOfCenterSurround, and the
 * bottom/proximity/wide speakers).
 */
[[nodiscard]]
constexpr std::optional<uint8_t>
surround_id_from_speaker (SpeakerArrangement::Speaker speaker)
{
  using Speaker = SpeakerArrangement::Speaker;
  switch (speaker)
    {
    case Speaker::Left:
      return CLAP_SURROUND_FL;
    case Speaker::Right:
      return CLAP_SURROUND_FR;
    case Speaker::Center:
      return CLAP_SURROUND_FC;
    case Speaker::Lfe:
      return CLAP_SURROUND_LFE;
    case Speaker::LeftSurround:
      return CLAP_SURROUND_BL;
    case Speaker::RightSurround:
      return CLAP_SURROUND_BR;
    case Speaker::LeftOfCenter:
      return CLAP_SURROUND_FLC;
    case Speaker::RightOfCenter:
      return CLAP_SURROUND_FRC;
    case Speaker::SurroundCenter:
      return CLAP_SURROUND_BC;
    case Speaker::SideLeft:
      return CLAP_SURROUND_SL;
    case Speaker::SideRight:
      return CLAP_SURROUND_SR;
    case Speaker::TopCenter:
      return CLAP_SURROUND_TC;
    case Speaker::TopFrontLeft:
      return CLAP_SURROUND_TFL;
    case Speaker::TopFrontCenter:
      return CLAP_SURROUND_TFC;
    case Speaker::TopFrontRight:
      return CLAP_SURROUND_TFR;
    case Speaker::TopRearLeft:
      return CLAP_SURROUND_TBL;
    case Speaker::TopRearCenter:
      return CLAP_SURROUND_TBC;
    case Speaker::TopRearRight:
      return CLAP_SURROUND_TBR;
    case Speaker::TopSideLeft:
      return CLAP_SURROUND_TSL;
    case Speaker::TopSideRight:
      return CLAP_SURROUND_TSR;
    default:
      return std::nullopt;
    }
}

/**
 * @brief Builds a speakers-kind arrangement from a CLAP surround channel map.
 *
 * @return std::nullopt when the map is empty, contains an unknown id, or names
 * a speaker twice.
 */
[[nodiscard]]
inline std::optional<SpeakerArrangement>
arrangement_from_surround_channel_map (std::span<const uint8_t> channel_map)
{
  if (channel_map.empty ())
    return std::nullopt;
  uint64_t bits = 0;
  for (const auto id : channel_map)
    {
      const auto speaker = speaker_from_surround_id (id);
      if (!speaker.has_value ())
        return std::nullopt;
      const auto bit = std::to_underlying (*speaker);
      if ((bits & bit) != 0)
        return std::nullopt;
      bits |= bit;
    }
  return SpeakerArrangement::from_speaker_bits (bits);
}

/**
 * @brief Permutation mapping plugin wire order to canonical channel order for
 * a surround channel map: plugin channel i carries the content of canonical
 * channel permutation[i].
 *
 * Canonical channel order is increasing speaker bit position (the VST3/SMPTE
 * convention the engine uses internally).
 *
 * @return std::nullopt when the map cannot be converted to a speaker
 * arrangement (see arrangement_from_surround_channel_map).
 */
[[nodiscard]]
inline std::optional<std::vector<uint8_t>>
surround_channel_permutation (std::span<const uint8_t> channel_map)
{
  const auto arrangement = arrangement_from_surround_channel_map (channel_map);
  if (!arrangement.has_value ())
    return std::nullopt;
  std::vector<uint8_t> permutation;
  permutation.reserve (channel_map.size ());
  for (const auto id : channel_map)
    {
      const auto bit = std::to_underlying (*speaker_from_surround_id (id));
      // the canonical index of a speaker is the number of speakers in the
      // arrangement with lower bit positions
      permutation.push_back (
        static_cast<uint8_t> (
          std::popcount (arrangement->speaker_bits () & (bit - 1))));
    }
  return permutation;
}

/**
 * @brief The dsp ambisonic ordering for a CLAP ordering constant.
 *
 * @return std::nullopt for an undefined constant.
 */
[[nodiscard]]
constexpr std::optional<SpeakerArrangement::AmbisonicOrdering>
ambisonic_ordering_from_clap (uint32_t ordering)
{
  switch (ordering)
    {
    case CLAP_AMBISONIC_ORDERING_FUMA:
      return SpeakerArrangement::AmbisonicOrdering::FuMa;
    case CLAP_AMBISONIC_ORDERING_ACN:
      return SpeakerArrangement::AmbisonicOrdering::Acn;
    default:
      return std::nullopt;
    }
}

/**
 * @brief The dsp ambisonic normalization for a CLAP normalization constant.
 *
 * @return std::nullopt for an undefined constant.
 */
[[nodiscard]]
constexpr std::optional<SpeakerArrangement::AmbisonicNormalization>
ambisonic_normalization_from_clap (uint32_t normalization)
{
  switch (normalization)
    {
    case CLAP_AMBISONIC_NORMALIZATION_MAXN:
      return SpeakerArrangement::AmbisonicNormalization::MaxN;
    case CLAP_AMBISONIC_NORMALIZATION_SN3D:
      return SpeakerArrangement::AmbisonicNormalization::Sn3d;
    case CLAP_AMBISONIC_NORMALIZATION_N3D:
      return SpeakerArrangement::AmbisonicNormalization::N3d;
    default:
      return std::nullopt;
    }
}

/**
 * @brief Builds an ambisonics arrangement from a CLAP ambisonic port
 * description.
 *
 * @return std::nullopt when the description has no honest representation:
 * unknown ordering or normalization constants, the horizontal-only schemes
 * (SN2D/N2D, which carry 2 * order + 1 channels), a channel count that is not
 * (order + 1)^2, or a FuMa ordering above 3rd order.
 */
[[nodiscard]]
constexpr std::optional<SpeakerArrangement>
arrangement_from_ambisonic_config (
  uint32_t channel_count,
  uint32_t ordering,
  uint32_t normalization)
{
  const auto dsp_ordering = ambisonic_ordering_from_clap (ordering);
  const auto dsp_normalization =
    ambisonic_normalization_from_clap (normalization);
  if (!dsp_ordering.has_value () || !dsp_normalization.has_value ())
    return std::nullopt;

  // Full-sphere schemes carry (order + 1)^2 channels: the count identifies
  // the order
  const auto orders = std::views::iota (
    0, static_cast<int> (SpeakerArrangement::kMaxAmbisonicOrder) + 1);
  const auto order_it =
    std::ranges::find_if (orders, [channel_count] (int order) {
      return static_cast<uint32_t> ((order + 1) * (order + 1)) == channel_count;
    });
  if (order_it == orders.end ())
    return std::nullopt;
  const auto order = *order_it;
  if (
    *dsp_ordering == SpeakerArrangement::AmbisonicOrdering::FuMa
    && order > SpeakerArrangement::kMaxFuMaAmbisonicOrder)
    return std::nullopt;
  return SpeakerArrangement::ambisonics (
    static_cast<uint8_t> (order), *dsp_ordering, *dsp_normalization);
}

/**
 * @brief The CLAP port type string for an arrangement, for
 * clap_audio_port_configuration_request.
 *
 * @return nullptr for Kind::Discrete (unspecified port type).
 */
[[nodiscard]]
inline const char *
port_type_from_arrangement (const SpeakerArrangement &arrangement)
{
  switch (arrangement.kind ())
    {
    case SpeakerArrangement::Kind::Speakers:
      if (arrangement.is_mono ())
        return CLAP_PORT_MONO;
      if (arrangement.is_stereo ())
        return CLAP_PORT_STEREO;
      return CLAP_PORT_SURROUND;
    case SpeakerArrangement::Kind::Ambisonics:
      return CLAP_PORT_AMBISONIC;
    case SpeakerArrangement::Kind::Discrete:
      return nullptr;
    }
  std::unreachable ();
}

/**
 * @brief Channel map in canonical order for a speakers arrangement, for
 * clap_audio_port_configuration_request port details.
 *
 * @return std::nullopt when the kind is not Kind::Speakers or a speaker has
 * no CLAP surround id.
 */
[[nodiscard]]
inline std::optional<std::vector<uint8_t>>
surround_channel_map_from_arrangement (const SpeakerArrangement &arrangement)
{
  if (arrangement.kind () != SpeakerArrangement::Kind::Speakers)
    return std::nullopt;
  std::vector<uint8_t> map;
  map.reserve (arrangement.channel_count ());
  for (
    const auto channel :
    std::views::iota (uint8_t{ 0 }, arrangement.channel_count ()))
    {
      const auto id =
        surround_id_from_speaker (*arrangement.channel_speaker (channel));
      if (!id.has_value ())
        return std::nullopt;
      map.push_back (*id);
    }
  return map;
}

/**
 * @brief The CLAP ambisonic ordering constant for a dsp ordering, for
 * clap_audio_port_configuration_request port details.
 */
[[nodiscard]]
constexpr uint32_t
ambisonic_ordering_to_clap (SpeakerArrangement::AmbisonicOrdering ordering)
{
  switch (ordering)
    {
    case SpeakerArrangement::AmbisonicOrdering::FuMa:
      return CLAP_AMBISONIC_ORDERING_FUMA;
    case SpeakerArrangement::AmbisonicOrdering::Acn:
      return CLAP_AMBISONIC_ORDERING_ACN;
    }
  std::unreachable ();
}

/**
 * @brief The CLAP ambisonic normalization constant for a dsp normalization,
 * for clap_audio_port_configuration_request port details.
 */
[[nodiscard]]
constexpr uint32_t
ambisonic_normalization_to_clap (
  SpeakerArrangement::AmbisonicNormalization normalization)
{
  switch (normalization)
    {
    case SpeakerArrangement::AmbisonicNormalization::MaxN:
      return CLAP_AMBISONIC_NORMALIZATION_MAXN;
    case SpeakerArrangement::AmbisonicNormalization::Sn3d:
      return CLAP_AMBISONIC_NORMALIZATION_SN3D;
    case SpeakerArrangement::AmbisonicNormalization::N3d:
      return CLAP_AMBISONIC_NORMALIZATION_N3D;
    }
  std::unreachable ();
}

/**
 * @brief The CLAP ambisonic ordering/normalization pair for an ambisonics
 * arrangement, for clap_audio_port_configuration_request port details.
 *
 * @return std::nullopt when the kind is not Kind::Ambisonics.
 */
[[nodiscard]]
constexpr std::optional<std::pair<uint32_t, uint32_t>>
ambisonic_config_from_arrangement (const SpeakerArrangement &arrangement)
{
  if (arrangement.kind () != SpeakerArrangement::Kind::Ambisonics)
    return std::nullopt;
  return std::pair{
    ambisonic_ordering_to_clap (arrangement.ambisonic_ordering ()),
    ambisonic_normalization_to_clap (arrangement.ambisonic_normalization ())
  };
}

} // namespace clap_speaker_arrangement

} // namespace zrythm::plugins
