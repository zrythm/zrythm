// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/fadeable_object.h"

namespace zrythm::structure::arrangement
{
ArrangerObjectFadeRange::ArrangerObjectFadeRange (
  const dsp::TempoMap &tempo_map,
  QObject *            parent)
    : QObject (parent), start_offset_ (tempo_map),
      start_offset_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (start_offset_, this)),
      end_offset_ (tempo_map),
      end_offset_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (end_offset_, this)),
      fade_in_opts_adapter_ (
        utils::make_qobject_unique<
          dsp::CurveOptionsQmlAdapter> (fade_in_opts_, this)),
      fade_out_opts_adapter_ (
        utils::make_qobject_unique<
          dsp::CurveOptionsQmlAdapter> (fade_out_opts_, this))
{
  QObject::connect (
    startOffset (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
  QObject::connect (
    endOffset (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
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
}
