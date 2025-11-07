// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/curve.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{
class ArrangerObjectFadeRange : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * startOffset READ startOffset CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * endOffset READ endOffset CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::CurveOptionsQmlAdapter * fadeInCurveOpts READ fadeInCurveOpts
      CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ArrangerObjectFadeRange (
    const dsp::AtomicPosition::TimeConversionFunctions &time_conversion_funcs,
    QObject *                                           parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  dsp::AtomicPositionQmlAdapter * startOffset () const
  {
    return start_offset_adapter_.get ();
  }
  dsp::AtomicPositionQmlAdapter * endOffset () const
  {
    return end_offset_adapter_.get ();
  }
  dsp::CurveOptionsQmlAdapter * fadeInCurveOpts () const
  {
    return fade_in_opts_adapter_.get ();
  }
  dsp::CurveOptionsQmlAdapter * fadeOutCurveOpts () const
  {
    return fade_out_opts_adapter_.get ();
  }

  Q_SIGNAL void fadePropertiesChanged ();

  // ========================================================================

  /**
   * Gets the normalized Y (ie, gain from 0-1) for a normalized X, for a fade.
   *
   * @param x Normalized x.
   * @param fade_in True for in, false for out.
   */
  double get_normalized_y_for_fade (double x, bool fade_in) const
  {
    if (fade_in)
      {
        return fadeInCurveOpts ()->normalizedY (x, false);
      }

    return fadeOutCurveOpts ()->normalizedY (x, true);
  }

protected:
  friend void init_from (
    ArrangerObjectFadeRange       &obj,
    const ArrangerObjectFadeRange &other,
    utils::ObjectCloneType         clone_type)
  {
    obj.start_offset_adapter_->setTicks (other.start_offset_adapter_->ticks ());
    obj.end_offset_adapter_->setTicks (other.end_offset_adapter_->ticks ());
    obj.fade_in_opts_ = other.fade_in_opts_;
    obj.fade_out_opts_ = other.fade_out_opts_;
  }

private:
  static constexpr auto kFadeInOffsetKey = "fadeInOffset"sv;
  static constexpr auto kFadeOutOffsetKey = "fadeOutOffset"sv;
  static constexpr auto kFadeInOptsKey = "fadeInOpts"sv;
  static constexpr auto kFadeOutOptsKey = "fadeOutOpts"sv;
  friend auto to_json (nlohmann::json &j, const ArrangerObjectFadeRange &object)
  {
    j[kFadeInOffsetKey] = object.start_offset_;
    j[kFadeOutOffsetKey] = object.end_offset_;
    j[kFadeInOptsKey] = object.fade_in_opts_;
    j[kFadeOutOptsKey] = object.fade_out_opts_;
  }
  friend auto
  from_json (const nlohmann::json &j, ArrangerObjectFadeRange &object)
  {
    j.at (kFadeInOffsetKey).get_to (object.start_offset_);
    j.at (kFadeOutOffsetKey).get_to (object.end_offset_);
    j.at (kFadeInOptsKey).get_to (object.fade_in_opts_);
    j.at (kFadeOutOptsKey).get_to (object.fade_out_opts_);
  }

  BOOST_DESCRIBE_CLASS (
    ArrangerObjectFadeRange,
    (),
    (),
    (),
    (start_offset_, end_offset_, fade_in_opts_, fade_out_opts_))

private:
  /** Fade start position (offset from object's start). */
  dsp::AtomicPosition                                    start_offset_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter> start_offset_adapter_;

  /**
   * Fade end position (offset from the object's end).
   */
  dsp::AtomicPosition                                    end_offset_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter> end_offset_adapter_;

  /** Fade in curve options. */
  dsp::CurveOptions                                    fade_in_opts_;
  utils::QObjectUniquePtr<dsp::CurveOptionsQmlAdapter> fade_in_opts_adapter_;

  /** Fade out curve options. */
  dsp::CurveOptions                                    fade_out_opts_;
  utils::QObjectUniquePtr<dsp::CurveOptionsQmlAdapter> fade_out_opts_adapter_;
};
} // namespace zrythm::structure::arrangement
