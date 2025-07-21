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
    dsp::AtomicPositionQmlAdapter * clipStartPosition READ clipStartPosition
      CONSTANT)
  Q_PROPERTY (
    dsp::AtomicPositionQmlAdapter * loopStartPosition READ loopStartPosition
      CONSTANT)
  Q_PROPERTY (
    dsp::AtomicPositionQmlAdapter * loopEndPosition READ loopEndPosition CONSTANT)
  Q_PROPERTY (
    bool trackLength READ trackLength WRITE setTrackLength NOTIFY
      trackLengthChanged)
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
  bool trackLength () const { return track_length_; }
  void setTrackLength (bool track)
  {
    if (track_length_ != track)
      {
        track_length_ = track;
        Q_EMIT trackLengthChanged (track);
      }
  }
  Q_SIGNAL void trackLengthChanged (bool track);

  // ========================================================================

  /**
   * Returns the length of the loop in ticks.
   */
  double get_loop_length_in_ticks () const
  {
    return std::max (
      0.0, loop_end_pos_.get_ticks () - loop_start_pos_.get_ticks ());
  }

  /**
   * Returns the length of the loop in frames.
   */
  [[gnu::hot]] auto get_loop_length_in_frames () const
  {
    return std::max (
      static_cast<int64_t> (0),
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
  static constexpr auto kTrackLengthKey = "trackLength"sv;
  friend auto to_json (nlohmann::json &j, const ArrangerObjectLoopRange &object)
  {
    j[kClipStartPosKey] = object.clip_start_pos_;
    j[kLoopStartPosKey] = object.loop_start_pos_;
    j[kLoopEndPosKey] = object.loop_end_pos_;
    j[kTrackLengthKey] = object.track_length_;
  }
  friend auto
  from_json (const nlohmann::json &j, ArrangerObjectLoopRange &object)
  {
    j.at (kClipStartPosKey).get_to (object.clip_start_pos_);
    j.at (kLoopStartPosKey).get_to (object.loop_start_pos_);
    j.at (kLoopEndPosKey).get_to (object.loop_end_pos_);
    j.at (kTrackLengthKey).get_to (object.track_length_);
    Q_EMIT object.trackLengthChanged (object.track_length_);
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
   * If true, loop end position will track (be set to) the object's length.
   */
  bool                                   track_length_{ true };
  std::optional<QMetaObject::Connection> track_length_connection_;

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
