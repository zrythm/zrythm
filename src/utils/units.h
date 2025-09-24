// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <mp-units/framework/quantity.h>
#include <mp-units/framework/quantity_spec.h>
#include <mp-units/framework/unit.h>
#include <mp-units/systems/isq.h>
#include <mp-units/systems/si.h>
#include <mp-units/systems/si/units.h>

namespace zrythm::units
{

namespace quantity_specs
{
inline constexpr struct sample_count
  final : mp_units::quantity_spec<mp_units::dimensionless, mp_units::is_kind>
{
} sample_count;
inline constexpr struct sample_rate final
    : mp_units::
        quantity_spec<mp_units::isq::frequency, sample_count / mp_units::isq::time>
{
} sample_rate;
inline constexpr struct tick_count
  final : mp_units::quantity_spec<mp_units::dimensionless, mp_units::is_kind>
{
} tick_count;

} // namespace quantity_specs

using precise_sample_rate_t =
  mp_units::quantity<quantity_specs::sample_rate[mp_units::si::hertz], double>;

inline constexpr struct sample final
    : mp_units::named_unit<
        "sample",
        mp_units::one,
        mp_units::kind_of<quantity_specs::sample_count>>
{
} sample;

using sample_t = mp_units::quantity<sample, int64_t>;
using precise_sample_t = mp_units::quantity<sample, double>;

inline constexpr struct tick final
    : mp_units::named_unit<
        "tick",
        mp_units::one,
        mp_units::kind_of<quantity_specs::tick_count>>
{
} tick;

using tick_t = mp_units::quantity<tick, int64_t>;
using precise_tick_t = mp_units::quantity<tick, double>;

constexpr tick_t                     PPQ = 960 * tick;
inline constexpr struct quarter_note final
    : mp_units::named_unit<
        "quarterNote",
        mp_units::mag<PPQ.numerical_value_in (tick)> * tick>
{
} quarter_note;

using precise_second_t = mp_units::quantity<mp_units::si::second, double>;
};
