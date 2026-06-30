// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/fadeable_object.h"

#include <au/math.hh>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
ArrangerObjectFadeRange::ArrangerObjectFadeRange (
  const dsp::Position        &start_position,
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  QObject *                   parent)
    : QObject (parent),
      start_offset_ (
        utils::make_qobject_unique<dsp::Position> (
          [] (units::precise_tick_t ticks) {
            return max (ticks, units::ticks (0.0));
          },
          this)),
      end_offset_ (
        utils::make_qobject_unique<dsp::Position> (
          [] (units::precise_tick_t ticks) {
            return max (ticks, units::ticks (0.0));
          },
          this)),
      fade_in_opts_adapter_ (
        utils::make_qobject_unique<
          dsp::CurveOptionsQmlAdapter> (fade_in_opts_, this)),
      fade_out_opts_adapter_ (
        utils::make_qobject_unique<
          dsp::CurveOptionsQmlAdapter> (fade_out_opts_, this))
{
  QObject::connect (
    startOffset (), &dsp::Position::positionChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
  QObject::connect (
    endOffset (), &dsp::Position::positionChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
  QObject::connect (
    fadeInCurveOpts (), &dsp::CurveOptionsQmlAdapter::algorithmChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
  QObject::connect (
    fadeInCurveOpts (), &dsp::CurveOptionsQmlAdapter::curvinessChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
  QObject::connect (
    fadeOutCurveOpts (), &dsp::CurveOptionsQmlAdapter::algorithmChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
  QObject::connect (
    fadeOutCurveOpts (), &dsp::CurveOptionsQmlAdapter::curvinessChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
}

void
to_json (nlohmann::json &j, const ArrangerObjectFadeRange &object)
{
  using T = ArrangerObjectFadeRange;
  j[T::kFadeInOffsetKey] = *object.start_offset_;
  j[T::kFadeOutOffsetKey] = *object.end_offset_;
  j[T::kFadeInOptsKey] = object.fade_in_opts_;
  j[T::kFadeOutOptsKey] = object.fade_out_opts_;
}

void
from_json (const nlohmann::json &j, ArrangerObjectFadeRange &object)
{
  using T = ArrangerObjectFadeRange;
  j.at (T::kFadeInOffsetKey).get_to (*object.start_offset_);
  j.at (T::kFadeOutOffsetKey).get_to (*object.end_offset_);
  j.at (T::kFadeInOptsKey).get_to (object.fade_in_opts_);
  j.at (T::kFadeOutOptsKey).get_to (object.fade_out_opts_);
}
}
