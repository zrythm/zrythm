// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>

#include "structure/arrangement/bounded_object.h"

using std::literals::string_view_literals::operator""sv;

namespace zrythm::structure::arrangement
{

class ArrangerObjectLoopRange : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * clipStartPosition READ
      clipStartPosition CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * loopStartPosition READ
      loopStartPosition CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * loopEndPosition READ loopEndPosition
      CONSTANT)
  Q_PROPERTY (
    bool trackBounds READ trackBounds WRITE setTrackBounds NOTIFY
      trackBoundsChanged)
  Q_PROPERTY (bool looped READ looped NOTIFY loopedChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ArrangerObjectLoopRange (
    const ArrangerObjectBounds &bounds,
    QObject *                   parent = nullptr);

  /**
   * Returns the number of loops in the ArrangerObject, optionally including
   * incomplete ones.
   */
  int get_num_loops (bool count_incomplete) const;

  // ========================================================================
  // QML Interface
  // ========================================================================

  dsp::AtomicPositionQmlAdapter * clipStartPosition () const
  {
    return clip_start_pos_adapter_.get ();
  }
  dsp::AtomicPositionQmlAdapter * loopStartPosition () const
  {
    return loop_start_pos_adapter_.get ();
  }
  dsp::AtomicPositionQmlAdapter * loopEndPosition () const
  {
    return loop_end_pos_adapter_.get ();
  }
  bool trackBounds () const { return track_bounds_; }
  void setTrackBounds (bool track)
  {
    if (track_bounds_ != track)
      {
        track_bounds_ = track;
        Q_EMIT trackBoundsChanged (track);
      }
  }
  Q_SIGNAL void trackBoundsChanged (bool track);

  bool looped () const
  {
    return loopStartPosition ()->samples () > 0
           || clipStartPosition ()->samples () > 0
           || length ()->samples () != loopEndPosition ()->samples ();
  }
  Q_SIGNAL void loopedChanged ();

  Q_SIGNAL void loopableObjectPropertiesChanged ();

  // ========================================================================

  /**
   * Returns the length of the loop in ticks.
   */
  units::precise_tick_t get_loop_length_in_ticks () const
  {
    return std::max (
      units::ticks (0.0),
      loop_end_pos_.get_ticks () - loop_start_pos_.get_ticks ());
  }

  /**
   * Returns the length of the loop in frames.
   */
  units::sample_t get_loop_length_in_frames () const
  {
    return std::max (
      units::sample_t (units::samples (0)),
      loop_end_pos_.get_samples () - loop_start_pos_.get_samples ());
  }

private:
  auto length () const -> const dsp::AtomicPositionQmlAdapter *
  {
    return bounds_.length ();
  }

  static constexpr auto kClipStartPosKey = "clipStartPosition"sv;
  static constexpr auto kLoopStartPosKey = "loopStartPosition"sv;
  static constexpr auto kLoopEndPosKey = "loopEndPosition"sv;
  static constexpr auto kTrackBoundsKey = "trackBounds"sv;
  friend void
  to_json (nlohmann::json &j, const ArrangerObjectLoopRange &object);
  friend void
  from_json (const nlohmann::json &j, ArrangerObjectLoopRange &object);

  friend void init_from (
    ArrangerObjectLoopRange       &obj,
    const ArrangerObjectLoopRange &other,
    utils::ObjectCloneType         clone_type)
  {
    obj.clip_start_pos_.set_ticks (other.clip_start_pos_.get_ticks ());
    obj.loop_start_pos_.set_ticks (other.loop_start_pos_.get_ticks ());
    obj.loop_end_pos_.set_ticks (other.loop_end_pos_.get_ticks ());
  }

  BOOST_DESCRIBE_CLASS (
    ArrangerObjectLoopRange,
    (),
    (),
    (),
    (clip_start_pos_, loop_start_pos_, loop_end_pos_))

private:
  /**
   * @brief Whether to track ArrangerObjectBounds::length().
   *
   * If true, all loop positions will track the bounds:
   * - Clip start position will be set to 0
   * - Loop start position will be set to 0
   * - Loop end position will be set to the object's length
   */
  bool                                   track_bounds_{ true };
  std::optional<QMetaObject::Connection> track_bounds_connection_;

  const ArrangerObjectBounds &bounds_;

  /**
   * Start position of the clip loop, relative to the object's start.
   *
   * The first time the object plays it will start playing from the
   * this position and then loop to @ref loop_start_pos_.
   */
  dsp::AtomicPosition clip_start_pos_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter> clip_start_pos_adapter_;

  /** Loop start Position relative to the object's start. */
  dsp::AtomicPosition loop_start_pos_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter> loop_start_pos_adapter_;

  /**
   * End position of the clip loop, relative to the object's start.
   *
   * Once this is reached, the clip will go back to @ref loop_start_pos_.
   */
  dsp::AtomicPosition                                    loop_end_pos_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter> loop_end_pos_adapter_;
};

} // namespace zrythm::structure::arrangement
