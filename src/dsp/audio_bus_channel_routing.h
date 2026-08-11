// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "dsp/panning.h"
#include "dsp/speaker_arrangement.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::dsp
{

/**
 * @brief A single source channel feeding a single destination channel.
 */
struct AudioBusChannelRoute
{
  uint8_t source_channel{};
  uint8_t destination_channel{};

  /** Gain applied to this contribution, on top of any connection multiplier. */
  float gain{ 1.f };

  constexpr bool operator== (const AudioBusChannelRoute &) const = default;
};

void
to_json (nlohmann::json &j, const AudioBusChannelRoute &route);
void
from_json (const nlohmann::json &j, AudioBusChannelRoute &route);

namespace detail
{
using Speaker = SpeakerArrangement::Speaker;

/** Gain applied when a speaker folds into a different speaker (-3 dB,
 * following ITU-R BS.775). */
constexpr float kSpeakerFoldGain = 0.70710678f;

/** Lateral side a speaker folds towards when the destination lacks it. */
enum class SpeakerSide : uint8_t
{
  Left,
  Right,
  Center,
  Lfe
};

constexpr SpeakerSide
speaker_side (Speaker speaker)
{
  switch (speaker)
    {
    case Speaker::Left:
    case Speaker::LeftSurround:
    case Speaker::LeftOfCenter:
    case Speaker::SideLeft:
    case Speaker::TopFrontLeft:
    case Speaker::TopRearLeft:
    case Speaker::TopSideLeft:
    case Speaker::LeftOfCenterSurround:
    case Speaker::BottomFrontLeft:
    case Speaker::ProximityLeft:
    case Speaker::BottomSideLeft:
    case Speaker::BottomRearLeft:
    case Speaker::LeftWide:
      return SpeakerSide::Left;
    case Speaker::Right:
    case Speaker::RightSurround:
    case Speaker::RightOfCenter:
    case Speaker::SideRight:
    case Speaker::TopFrontRight:
    case Speaker::TopRearRight:
    case Speaker::TopSideRight:
    case Speaker::RightOfCenterSurround:
    case Speaker::BottomFrontRight:
    case Speaker::ProximityRight:
    case Speaker::BottomSideRight:
    case Speaker::BottomRearRight:
    case Speaker::RightWide:
      return SpeakerSide::Right;
    case Speaker::Lfe:
    case Speaker::Lfe2:
      return SpeakerSide::Lfe;
    default:
      return SpeakerSide::Center;
    }
}

/**
 * Whether the speaker belongs to the surround/immersive family, which
 * prefers folding into a same-side surround speaker when the destination
 * has one (front speakers fold straight to the front).
 */
constexpr bool
is_surround_family (Speaker speaker)
{
  switch (speaker)
    {
    case Speaker::LeftSurround:
    case Speaker::RightSurround:
    case Speaker::SurroundCenter:
    case Speaker::SideLeft:
    case Speaker::SideRight:
    case Speaker::TopRearLeft:
    case Speaker::TopRearCenter:
    case Speaker::TopRearRight:
    case Speaker::TopSideLeft:
    case Speaker::TopSideRight:
    case Speaker::LeftOfCenterSurround:
    case Speaker::RightOfCenterSurround:
    case Speaker::BottomSideLeft:
    case Speaker::BottomSideRight:
    case Speaker::BottomRearLeft:
    case Speaker::BottomRearCenter:
    case Speaker::BottomRearRight:
      return true;
    default:
      return false;
    }
}

/** Same-side surround speakers a left surround folds into, in priority
 * order (the destination's front left is always the final target). */
constexpr std::array kLeftSurroundFoldTargets{
  Speaker::LeftSurround,        Speaker::SideLeft,
  Speaker::TopRearLeft,         Speaker::TopSideLeft,
  Speaker::BottomSideLeft,      Speaker::BottomRearLeft,
  Speaker::LeftOfCenterSurround
};
constexpr std::array kRightSurroundFoldTargets{
  Speaker::RightSurround,        Speaker::SideRight,
  Speaker::TopRearRight,         Speaker::TopSideRight,
  Speaker::BottomSideRight,      Speaker::BottomRearRight,
  Speaker::RightOfCenterSurround
};

constexpr bool
has (uint64_t bits, Speaker speaker)
{
  return (bits & std::to_underlying (speaker)) != 0;
}

/** First speaker from @p candidates present in @p bits, if any. */
constexpr std::optional<Speaker>
first_present (uint64_t bits, std::span<const Speaker> candidates)
{
  const auto it = std::ranges::find_if (candidates, [&] (Speaker candidate) {
    return has (bits, candidate);
  });
  return it == candidates.end () ? std::nullopt : std::optional{ *it };
}

/** Channel index of @p speaker within the channel-ordered mask @p bits. */
constexpr unsigned
channel_index_of (uint64_t bits, Speaker speaker)
{
  const auto bit = std::to_underlying (speaker);
  return static_cast<unsigned> (std::popcount (bits & (bit - 1)));
}

/**
 * Per-speaker folding between two speaker masks (ITU-R BS.775 downmix
 * rules generalized to arbitrary pairs):
 *
 * - a speaker present in the destination routes to itself at unity
 * - the center folds to the destination center, or to the front pair at
 *   -3 dB
 * - mono is treated as center content: it routes to the destination center
 *   at unity, or to the front pair at -3 dB
 * - rear-center speakers fold to the first available surround speaker on
 *   each side at -3 dB (only when both sides have one, so the image stays
 *   centered), then follow the center rules
 * - surround speakers fold to a same-side surround at -3 dB, or to the
 *   same-side front speaker at -3 dB
 * - front speakers fold to the same-side front speaker at -3 dB
 * - a mono destination sums the front pair at the center pan gains (-6 dB
 *   law) and the center at -3 dB relative to the front pair, drops the
 *   surrounds and discards LFE
 * - LFE is intentionally discarded (per BS.775) and is not reported as
 *   dropped content
 */
template <typename RouteFunc, typename DropFunc>
void
for_each_speaker_fold_route (
  uint64_t    src_bits,
  uint64_t    dest_bits,
  RouteFunc &&route,
  DropFunc  &&drop)
{
  // a mono destination sums the front pair and center; surrounds are
  // dropped
  if (dest_bits == std::to_underlying (Speaker::Mono))
    {
      const auto front_gains =
        calculate_panning (PanLaw::Minus6dB, PanAlgorithm::SquareRoot, 0.5f);
      unsigned src_ch = 0;
      for (uint64_t remaining = src_bits; remaining != 0;)
        {
          const auto bit = remaining & (~remaining + 1);
          remaining &= remaining - 1;
          const auto speaker = static_cast<Speaker> (bit);
          const auto this_src_ch = src_ch++;
          if (speaker == Speaker::Mono)
            {
              route (0u, this_src_ch, 1.f);
              continue;
            }
          const auto side = speaker_side (speaker);
          switch (side)
            {
            case SpeakerSide::Lfe:
              // discarded by design (see function comment)
              break;
            case SpeakerSide::Center:
            case SpeakerSide::Left:
            case SpeakerSide::Right:
              if (is_surround_family (speaker))
                {
                  drop (this_src_ch);
                }
              else if (side == SpeakerSide::Center)
                {
                  // -3 dB relative to the front pair, keeping the same
                  // center-to-front ratio as a surround-to-stereo fold
                  route (0u, this_src_ch, front_gains.first * kSpeakerFoldGain);
                }
              else
                {
                  route (
                    0u, this_src_ch,
                    side == SpeakerSide::Left
                      ? front_gains.first
                      : front_gains.second);
                }
              break;
            }
        }
      return;
    }

  const auto fold_to = [&] (Speaker target, unsigned src_ch) {
    route (channel_index_of (dest_bits, target), src_ch, kSpeakerFoldGain);
  };

  unsigned src_ch = 0;
  for (uint64_t remaining = src_bits; remaining != 0;)
    {
      const auto bit = remaining & (~remaining + 1);
      remaining &= remaining - 1;
      const auto speaker = static_cast<Speaker> (bit);
      const auto this_src_ch = src_ch++;
      if (has (dest_bits, speaker))
        {
          route (channel_index_of (dest_bits, speaker), this_src_ch, 1.f);
          continue;
        }
      const auto side = speaker_side (speaker);
      switch (side)
        {
        case SpeakerSide::Lfe:
          // discarded by design (see function comment)
          break;
        case SpeakerSide::Center:
          {
            // rear-center speakers fold into the first available surround
            // speaker on each side, but only when both sides have one so
            // the image stays centered
            if (is_surround_family (speaker))
              {
                const auto left_target =
                  first_present (dest_bits, kLeftSurroundFoldTargets);
                const auto right_target =
                  first_present (dest_bits, kRightSurroundFoldTargets);
                if (left_target.has_value () && right_target.has_value ())
                  {
                    fold_to (*left_target, this_src_ch);
                    fold_to (*right_target, this_src_ch);
                    break;
                  }
              }
            if (has (dest_bits, Speaker::Center))
              {
                // mono content keeps unity when it lands on a single speaker
                if (speaker == Speaker::Mono)
                  {
                    route (
                      channel_index_of (dest_bits, Speaker::Center),
                      this_src_ch, 1.f);
                  }
                else
                  {
                    fold_to (Speaker::Center, this_src_ch);
                  }
              }
            else if (
              has (dest_bits, Speaker::Left) && has (dest_bits, Speaker::Right))
              {
                fold_to (Speaker::Left, this_src_ch);
                fold_to (Speaker::Right, this_src_ch);
              }
            else
              {
                drop (this_src_ch);
              }
          }
          break;
        case SpeakerSide::Left:
        case SpeakerSide::Right:
          {
            const bool is_left = side == SpeakerSide::Left;
            const auto front = is_left ? Speaker::Left : Speaker::Right;
            const auto surround_target =
              is_surround_family (speaker)
                ? first_present (
                    dest_bits,
                    is_left ? kLeftSurroundFoldTargets : kRightSurroundFoldTargets)
                : std::nullopt;
            if (surround_target.has_value ())
              {
                fold_to (*surround_target, this_src_ch);
              }
            else if (has (dest_bits, front))
              {
                fold_to (front, this_src_ch);
              }
            else
              {
                drop (this_src_ch);
              }
          }
          break;
        }
    }
}

/**
 * Worker for for_each_derived_route() that additionally reports source
 * channels whose content is discarded via @p drop.
 */
template <typename RouteFunc, typename DropFunc>
void
for_each_derived_route_impl (
  const SpeakerArrangement &src,
  const SpeakerArrangement &dest,
  RouteFunc               &&route,
  DropFunc                &&drop)
{
  const auto src_channels = src.channel_count ();
  const auto dest_channels = dest.channel_count ();

  const auto one_for_one = [&] (uint8_t count) {
    for (const auto ch : std::views::iota (0u, static_cast<unsigned> (count)))
      {
        route (ch, ch, 1.f);
      }
  };

  const auto broadcast_first_source_channel = [&] {
    for (
      const auto ch :
      std::views::iota (0u, static_cast<unsigned> (dest_channels)))
      {
        route (ch, 0u, 1.f);
      }
  };

  const auto sum_first_source_pair = [&] {
    const auto gains =
      calculate_panning (PanLaw::Minus6dB, PanAlgorithm::SquareRoot, 0.5f);
    route (0u, 0u, gains.first);
    route (0u, 1u, gains.second);
    // content beyond the first pair is discarded
    for (
      const auto ch :
      std::views::iota (2u, static_cast<unsigned> (src_channels)))
      {
        drop (ch);
      }
  };

  if (
    src.kind () == SpeakerArrangement::Kind::Speakers
    && dest.kind () == SpeakerArrangement::Kind::Speakers)
    {
      // speaker semantics take priority: identical layouts route channel
      // for channel, anything else folds per speaker
      if (src.speaker_bits () == dest.speaker_bits ())
        {
          one_for_one (dest_channels);
        }
      else
        {
          for_each_speaker_fold_route (
            src.speaker_bits (), dest.speaker_bits (),
            std::forward<RouteFunc> (route), std::forward<DropFunc> (drop));
        }
    }
  else if (src_channels == dest_channels)
    {
      one_for_one (dest_channels);
    }
  else if (src_channels == 1)
    {
      broadcast_first_source_channel ();
    }
  else if (dest_channels == 1 && src_channels >= 2)
    {
      sum_first_source_pair ();
    }
  else
    {
      const auto fed = std::min (src_channels, dest_channels);
      one_for_one (fed);
      // content beyond the destination's channel count is discarded
      for (
        const auto ch : std::views::iota (
          static_cast<unsigned> (fed), static_cast<unsigned> (src_channels)))
        {
          drop (ch);
        }
    }
}

} // namespace detail

/**
 * @brief Invokes @p route with (destination_channel, source_channel, gain) for
 * each contribution in the default routing from @p src to @p dest.
 *
 * Speaker semantics take priority; channel counts are only consulted for
 * combinations the arrangements can't describe (most commonly Kind::Discrete,
 * reported by plugins that don't publish a layout):
 *
 * - both speaker layouts: identical layouts route channel for channel,
 *   anything else folds per speaker (see
 *   detail::for_each_speaker_fold_route)
 * - equal channel counts: channel for channel
 * - any single-channel source: source channel 0 to every destination channel
 * - any source with 2 or more channels feeding a single-channel destination:
 *   the first two source channels summed at -6dB
 * - anything else: the first min(source, destination) channels, one for one
 *
 * Destination channels that receive no contribution are left for the caller to
 * handle, since that differs between accumulating and overwriting.
 */
template <typename RouteFunc>
void
for_each_derived_route (
  const SpeakerArrangement &src,
  const SpeakerArrangement &dest,
  RouteFunc               &&route)
{
  detail::for_each_derived_route_impl (
    src, dest, std::forward<RouteFunc> (route), [] (unsigned) { });
}

/**
 * @brief Whether the derived routing from @p src to @p dest discards or
 * mis-maps source content.
 *
 * Used to warn when a connection silently loses channels: the routing falls
 * back to channel counts and the source has more channels than the
 * destination, or speaker folding drops a speaker the destination can't
 * represent. Ambisonics always counts: one side being ambisonics and the
 * other not, or both sides using different channel ordering or
 * normalization conventions, has no honest channel mapping until dedicated
 * decode lands. LFE discarded by the speaker folding rules is intentional
 * and does not count as dropped content.
 */
[[nodiscard]] inline bool
derived_routing_drops_content (
  const SpeakerArrangement &src,
  const SpeakerArrangement &dest)
{
  const auto ambisonics = SpeakerArrangement::Kind::Ambisonics;
  const bool src_ambisonics = src.kind () == ambisonics;
  const bool dest_ambisonics = dest.kind () == ambisonics;
  if (src_ambisonics != dest_ambisonics)
    {
      return true;
    }
  if (
    src_ambisonics
    && (src.ambisonic_ordering () != dest.ambisonic_ordering ()
        || src.ambisonic_normalization () != dest.ambisonic_normalization ()))
    {
      // channel for channel between different conventions mis-maps content
      return true;
    }

  bool drops = false;
  detail::for_each_derived_route_impl (
    src, dest, [] (unsigned, unsigned, float) { },
    [&drops] (unsigned) { drops = true; });
  return drops;
}

/**
 * @brief How the channels of a source bus feed the channels of a destination
 * bus.
 *
 * Defaults to deriving the routing from the two speaker arrangements (see
 * @ref for_each_derived_route). An explicit set of routes replaces that
 * entirely, which is what a per-connection channel matrix edits. An explicit
 * routing may be empty, meaning no channel of the source reaches the
 * destination.
 */
class AudioBusChannelRouting
{
public:
  /** Routing derived from the speaker arrangements. */
  AudioBusChannelRouting () = default;

  /**
   * @brief Explicit routing.
   *
   * @throw std::invalid_argument if two routes describe the same source and
   * destination channel pair.
   */
  explicit AudioBusChannelRouting (std::vector<AudioBusChannelRoute> routes);

  [[nodiscard]] bool is_derived () const { return !routes_.has_value (); }

  /** Explicit routes, or an empty span when the routing is derived. */
  [[nodiscard]] std::span<const AudioBusChannelRoute> routes () const
  {
    return routes_.has_value ()
             ? std::span{ *routes_ }
             : std::span<const AudioBusChannelRoute>{};
  }

  /** Reverts to routing derived from the speaker arrangements. */
  void reset_to_derived () { routes_.reset (); }

  /**
   * @brief Invokes @p route with (destination_channel, source_channel, gain)
   * for each contribution needed to feed @p dest from @p src.
   *
   * Routes referring to channels beyond either arrangement are skipped, so an
   * explicit routing stays safe to apply after a port's arrangement changes.
   */
  template <typename RouteFunc>
  void for_each_route (
    const SpeakerArrangement &src,
    const SpeakerArrangement &dest,
    RouteFunc               &&route) const
  {
    if (!routes_.has_value ())
      {
        for_each_derived_route (src, dest, route);
        return;
      }

    for (const auto &explicit_route : *routes_)
      {
        if (
          explicit_route.source_channel >= src.channel_count ()
          || explicit_route.destination_channel >= dest.channel_count ())
          continue;
        route (
          static_cast<unsigned> (explicit_route.destination_channel),
          static_cast<unsigned> (explicit_route.source_channel),
          explicit_route.gain);
      }
  }

  /**
   * @brief Whether feeding @p dest from @p src writes every destination
   * channel exactly once, at unity gain, from the source channel of the same
   * index.
   *
   * Lets callers that overwrite the destination skip clearing it first.
   */
  [[nodiscard]] bool is_channel_for_channel (
    const SpeakerArrangement &src,
    const SpeakerArrangement &dest) const;

  bool operator== (const AudioBusChannelRouting &) const = default;

private:
  friend void
  to_json (nlohmann::json &j, const AudioBusChannelRouting &routing);
  friend void
  from_json (const nlohmann::json &j, AudioBusChannelRouting &routing);

  /** Unset means derived from the speaker arrangements. */
  std::optional<std::vector<AudioBusChannelRoute>> routes_;
};

void
to_json (nlohmann::json &j, const AudioBusChannelRouting &routing);
void
from_json (const nlohmann::json &j, AudioBusChannelRouting &routing);

/**
 * @brief fmt printing support (for logging).
 */
[[nodiscard]] std::string
format_as (const AudioBusChannelRoute &route);
[[nodiscard]] std::string
format_as (const AudioBusChannelRouting &routing);

} // namespace zrythm::dsp
