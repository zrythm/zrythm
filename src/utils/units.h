// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <au/au.hh>
#include <fmt/format.h>

namespace zrythm::units
{

// Define base dimensions using unique indices based on Unix epoch timestamps
// for uniqueness
struct SampleBaseDim : au::base_dim::BaseDimension<1737814550>
{
};
struct TickBaseDim : au::base_dim::BaseDimension<1737814551>
{
};
struct ChannelBaseDim : au::base_dim::BaseDimension<1777738085>
{
};

// Define sample unit with its own dimension
struct Sample : au::UnitImpl<au::Dimension<SampleBaseDim>>
{
  static constexpr const char label[] = "sample";
};
constexpr auto sample = au::SingularNameFor<Sample>{};
constexpr auto samples = au::QuantityMaker<Sample>{};

// Define sample quantity types
using sample_t = au::QuantityI64<Sample>;
using sample_u32_t = au::QuantityU32<Sample>;
using sample_u64_t = au::QuantityU64<Sample>;
using precise_sample_t = au::QuantityD<Sample>;

// Define tick unit with its own dimension
struct Tick : au::UnitImpl<au::Dimension<TickBaseDim>>
{
  static constexpr const char label[] = "tick";
};
constexpr auto tick = au::SingularNameFor<Tick>{};
constexpr auto ticks = au::QuantityMaker<Tick>{};

// Define tick quantity types
using tick_t = au::QuantityI64<Tick>;
using precise_tick_t = au::QuantityD<Tick>;

// Define channel count unit with its own dimension
struct ChannelCount : au::UnitImpl<au::Dimension<ChannelBaseDim>>
{
  static constexpr const char label[] = "channel";
};
constexpr auto channel = au::SingularNameFor<ChannelCount>{};
constexpr auto channels = au::QuantityMaker<ChannelCount>{};

// Define channel count quantity types
using channel_count_t = au::Quantity<ChannelCount, uint8_t>;

// Define PPQ constant (960 ticks per quarter note)
constexpr tick_t PPQ = ticks (960);

// Define quarter_note unit as 960 ticks using unit expression
struct QuarterNote : decltype (Tick{} * au::mag<960> ())
{
  static constexpr const char label[] = "quarterNote";
};
constexpr auto quarter_note = au::SingularNameFor<QuarterNote>{};
constexpr auto quarter_notes = au::QuantityMaker<QuarterNote>{};

// Define sample rate as a compound unit (samples per second)
using SampleRate = decltype (Sample{} / au::Seconds{});
constexpr auto sample_rate = samples / au::second;
using sample_rate_t = au::QuantityI<SampleRate>;
using precise_sample_rate_t = au::QuantityD<SampleRate>;

// Define precise second using Au's built-in seconds
constexpr auto seconds = au::seconds;
using precise_second_t = au::QuantityD<au::Seconds>;
} // namespace zrythm::units

// Formatter for Au quantities
template <typename U, typename R>
struct fmt::formatter<::au::Quantity<U, R>>
    : ::au::QuantityFormatter<U, R, ::fmt::formatter>
{
};
