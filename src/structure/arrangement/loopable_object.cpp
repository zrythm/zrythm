// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/loopable_object.h"

namespace zrythm::structure::arrangement
{
ArrangerObjectLoopRange::ArrangerObjectLoopRange (
  const ArrangerObjectBounds &bounds,
  QObject *                   parent)
    : QObject (parent), bounds_ (bounds),
      clip_start_pos_ (bounds.length ()->position ().get_tempo_map ()),
      clip_start_pos_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (clip_start_pos_, this)),
      loop_start_pos_ (bounds.length ()->position ().get_tempo_map ()),
      loop_start_pos_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (loop_start_pos_, this)),
      loop_end_pos_ (bounds.length ()->position ().get_tempo_map ()),
      loop_end_pos_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (loop_end_pos_, this))
{
  QObject::connect (
    this, &ArrangerObjectLoopRange::trackLengthChanged, this,
    [this] (bool track) {
      if (track && !track_length_connection_.has_value ())
        {
          track_length_connection_ = QObject::connect (
            bounds_.length (), &dsp::AtomicPositionQmlAdapter::positionChanged,
            this,
            [this] () { loopEndPosition ()->setTicks (length ()->ticks ()); });

          // also emit the signal to update the loop end position since we are
          // now tracking the bounds
          Q_EMIT bounds_.length ()->positionChanged ();
        }
      else if (!track && track_length_connection_.has_value ())
        {
          QObject::disconnect (track_length_connection_.value ());
          track_length_connection_.reset ();
        }
    });

  Q_EMIT trackLengthChanged (track_length_);
}

int
ArrangerObjectLoopRange::get_num_loops (bool count_incomplete) const
{
  const auto full_size = length ()->samples ();
  const auto loop_start =
    loopStartPosition ()->samples () - clipStartPosition ()->samples ();
  const auto loop_size = get_loop_length_in_frames ();

  // Special case
  if (loop_size == 0) [[unlikely]]
    {
      return 0;
    }

  // Calculate the number of full loops using integer division
  const auto full_loops = (full_size - loop_start) / loop_size;

  // Add 1 if we want to count incomplete loops
  const auto add_one =
    (count_incomplete && (full_size - loop_start) % loop_size != 0);
  return static_cast<int> (full_loops) + (add_one ? 1 : 0);
}
}
