// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>
#include <string_view>

#include "dsp/audio_bus_channel_routing.h"
#include "utils/math_utils.h"
#include "utils/views.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

using namespace std::string_view_literals;

namespace zrythm::dsp
{

namespace
{
constexpr auto kRoutesKey = "routes"sv;
constexpr auto kSourceChannelKey = "sourceChannel"sv;
constexpr auto kDestinationChannelKey = "destinationChannel"sv;
constexpr auto kGainKey = "gain"sv;
}

AudioBusChannelRouting::AudioBusChannelRouting (
  std::vector<AudioBusChannelRoute> routes)
{
  for (const auto &[index, route] : routes | utils::views::enumerate)
    {
      const auto duplicate = std::ranges::any_of (
        routes | std::views::take (index), [&route] (const auto &earlier) {
          return earlier.source_channel == route.source_channel
                 && earlier.destination_channel == route.destination_channel;
        });
      if (duplicate)
        {
          throw std::invalid_argument (
            fmt::format (
              "Duplicate channel route {} -> {}", route.source_channel,
              route.destination_channel));
        }
    }
  routes_ = std::move (routes);
}

bool
AudioBusChannelRouting::is_channel_for_channel (
  const SpeakerArrangement &src,
  const SpeakerArrangement &dest) const
{
  if (src.channel_count () != dest.channel_count ())
    return false;

  if (!routes_.has_value ())
    {
      // a derived routing between speaker layouts folds per speaker, so it
      // is only channel for channel when the layouts are identical
      if (
        src.kind () == SpeakerArrangement::Kind::Speakers
        && dest.kind () == SpeakerArrangement::Kind::Speakers)
        {
          return src.speaker_bits () == dest.speaker_bits ();
        }
      return true;
    }

  if (routes_->size () != dest.channel_count ())
    return false;

  return std::ranges::all_of (*routes_, [] (const auto &route) {
    return route.source_channel == route.destination_channel
           && utils::math::floats_near (route.gain, 1.f, 0.00001f);
  });
}

std::string
format_as (const AudioBusChannelRoute &route)
{
  return fmt::format (
    "{}->{}@{:.2f}", route.source_channel, route.destination_channel,
    route.gain);
}

std::string
format_as (const AudioBusChannelRouting &routing)
{
  if (routing.is_derived ())
    return "derived";

  std::string result = "[";
  for (const auto &[index, route] : routing.routes () | utils::views::enumerate)
    {
      if (index > 0)
        result += ", ";
      result += format_as (route);
    }
  result += ']';
  return result;
}

static_assert (fmt::formattable<AudioBusChannelRoute>);
static_assert (fmt::formattable<AudioBusChannelRouting>);

void
to_json (nlohmann::json &j, const AudioBusChannelRoute &route)
{
  j = nlohmann::json{
    { kSourceChannelKey,      route.source_channel      },
    { kDestinationChannelKey, route.destination_channel },
    { kGainKey,               route.gain                }
  };
}

void
from_json (const nlohmann::json &j, AudioBusChannelRoute &route)
{
  j.at (kSourceChannelKey).get_to (route.source_channel);
  j.at (kDestinationChannelKey).get_to (route.destination_channel);
  j.at (kGainKey).get_to (route.gain);
}

void
to_json (nlohmann::json &j, const AudioBusChannelRouting &routing)
{
  j = nlohmann::json::object ();
  if (routing.routes_.has_value ())
    {
      j[kRoutesKey] = *routing.routes_;
    }
}

void
from_json (const nlohmann::json &j, AudioBusChannelRouting &routing)
{
  routing.routes_.reset ();
  if (!j.contains (kRoutesKey))
    return;

  // reuse the constructor so that deserialized routings get the same validation
  // as ones built in-process
  routing = AudioBusChannelRouting{
    j.at (kRoutesKey).get<std::vector<AudioBusChannelRoute>> ()
  };
}

} // namespace zrythm::dsp
