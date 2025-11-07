// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/bounded_object.h"

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
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ArrangerObjectLoopRange (
    const ArrangerObjectBounds &bounds,
    QObject *                   parent = nullptr);

  bool is_looped () const
  {
    return loopStartPosition ()->samples () > 0
           || clipStartPosition ()->samples () > 0
           || length ()->samples () != loopEndPosition ()->samples ();
  }

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

  static constexpr auto kClipStartPosKey = "clipStartPos"sv;
  static constexpr auto kLoopStartPosKey = "loopStartPos"sv;
  static constexpr auto kLoopEndPosKey = "loopEndPos"sv;
  static constexpr auto kTrackBoundsKey = "trackBounds"sv;
  friend auto to_json (nlohmann::json &j, const ArrangerObjectLoopRange &object)
  {
    j[kClipStartPosKey] = object.clip_start_pos_;
    j[kLoopStartPosKey] = object.loop_start_pos_;
    j[kLoopEndPosKey] = object.loop_end_pos_;
    j[kTrackBoundsKey] = object.track_bounds_;
  }
  friend auto
  from_json (const nlohmann::json &j, ArrangerObjectLoopRange &object)
  {
    j.at (kClipStartPosKey).get_to (object.clip_start_pos_);
    j.at (kLoopStartPosKey).get_to (object.loop_start_pos_);
    j.at (kLoopEndPosKey).get_to (object.loop_end_pos_);
    j.at (kTrackBoundsKey).get_to (object.track_bounds_);
    Q_EMIT object.trackBoundsChanged (object.track_bounds_);
  }

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
