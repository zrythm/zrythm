// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string>
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

/**
 * @brief Invokes @p route with (destination_channel, source_channel, gain) for
 * each contribution in the default routing from @p src to @p dest.
 *
 * Speaker semantics take priority; channel counts are only consulted for
 * combinations the arrangements can't describe (most commonly Kind::Discrete,
 * reported by plugins that don't publish a layout):
 *
 * - equal channel counts: channel for channel
 * - mono to stereo, or any single-channel source: source channel 0 to every
 *   destination channel
 * - stereo to mono, or any source with 2 or more channels feeding a
 *   single-channel destination: the first two source channels summed at -6dB
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
  };

  if (src_channels == dest_channels)
    {
      one_for_one (dest_channels);
    }
  else if (src.is_mono () && dest.is_stereo ())
    {
      broadcast_first_source_channel ();
    }
  else if (src.is_stereo () && dest.is_mono ())
    {
      sum_first_source_pair ();
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
      one_for_one (std::min (src_channels, dest_channels));
    }
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
