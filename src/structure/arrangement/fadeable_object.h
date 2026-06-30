// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/curve.h"
#include "dsp/position.h"
#include "utils/icloneable.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::arrangement
{
class ArrangerObjectFadeRange : public QObject
{
  Q_OBJECT
  Q_PROPERTY (zrythm::dsp::Position * startOffset READ startOffset CONSTANT)
  Q_PROPERTY (zrythm::dsp::Position * endOffset READ endOffset CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::CurveOptionsQmlAdapter * fadeInCurveOpts READ fadeInCurveOpts
      CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::CurveOptionsQmlAdapter * fadeOutCurveOpts READ fadeOutCurveOpts
      CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ArrangerObjectFadeRange (
    const dsp::Position        &start_position,
    const dsp::TempoMapWrapper &tempo_map_wrapper,
    QObject *                   parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  dsp::Position * startOffset () const { return start_offset_.get (); }
  dsp::Position * endOffset () const { return end_offset_.get (); }
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
    obj.start_offset_->setTicks (other.start_offset_->ticks ());
    obj.end_offset_->setTicks (other.end_offset_->ticks ());
    obj.fade_in_opts_ = other.fade_in_opts_;
    obj.fade_out_opts_ = other.fade_out_opts_;
  }

private:
  static constexpr auto kFadeInOffsetKey = "fadeInOffset"sv;
  static constexpr auto kFadeOutOffsetKey = "fadeOutOffset"sv;
  static constexpr auto kFadeInOptsKey = "fadeInOpts"sv;
  static constexpr auto kFadeOutOptsKey = "fadeOutOpts"sv;
  friend void
  to_json (nlohmann::json &j, const ArrangerObjectFadeRange &object);
  friend void
  from_json (const nlohmann::json &j, ArrangerObjectFadeRange &object);

  BOOST_DESCRIBE_CLASS (
    ArrangerObjectFadeRange,
    (),
    (),
    (),
    (start_offset_, end_offset_, fade_in_opts_, fade_out_opts_))

private:
  /**
   * @brief Fade-in end position, as content ticks from the clip start.
   *
   * Unlike loop bounds and note positions, fade offsets are NOT warped through
   * ContentTimeWarp. They represent plain project-tick offsets from the clip
   * start, applied to the already-stretched output audio.
   *
   * @note Currently tick-based only. Future work may add a seconds-based mode
   * (for real-time fade durations independent of tempo). The most natural
   * extension would be to store @c units::precise_second_t internally and
   * derive ticks for display via the tempo map — or a @c FadeDuration type
   * wrapping a tick/second variant with a user-selectable mode. Plain
   * @c Position today does not constrain either option.
   */
  utils::QObjectUniquePtr<dsp::Position> start_offset_;

  /**
   * @brief Fade-out start position.
   *
   * @note Currently stored as ticks offset from the object's start. This may be
   * redefined as an absolute content position (ticks from clip start) in a
   * future change. Not a @c ContentPosition because fades are not warped —
   * see @ref start_offset_ for details.
   */
  utils::QObjectUniquePtr<dsp::Position> end_offset_;

  /** Fade in curve options. */
  dsp::CurveOptions                                    fade_in_opts_;
  utils::QObjectUniquePtr<dsp::CurveOptionsQmlAdapter> fade_in_opts_adapter_;

  /** Fade out curve options. */
  dsp::CurveOptions                                    fade_out_opts_;
  utils::QObjectUniquePtr<dsp::CurveOptionsQmlAdapter> fade_out_opts_adapter_;
};
} // namespace zrythm::structure::arrangement
