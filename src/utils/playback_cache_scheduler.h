// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/expandable_tick_range.h"

#include <QTimer>

namespace zrythm::utils
{
/**
 * @brief Cache request handler for a tick range, with built-in debouncing and
 * expanding of the range.
 *
 * @note A debouncer activates the slot only once, after a timeout / grace
 * period calculated since the last signal emission. In other words: if a signal
 * keeps coming, the slot is not activated. Use case: a search box that actually
 * starts searching only after the user stops typing (that is, after a short
 * timeout since the last user input).
 */
class PlaybackCacheScheduler : public QObject
{
  Q_OBJECT

public:
  PlaybackCacheScheduler (QObject * parent = nullptr) : QObject (parent)
  {
    timer_.setSingleShot (true);
    connect (
      &timer_, &QTimer::timeout, this,
      &PlaybackCacheScheduler::executePendingCall);
  }

  Q_INVOKABLE void
  queueCacheRequestForRange (double affectedTickStart, double affectedTickEnd)
  {
    queueCacheRequest ({ std::make_pair (affectedTickStart, affectedTickEnd) });
  }

  Q_INVOKABLE void queueFullCacheRequest () { queueCacheRequest ({}); }

  Q_INVOKABLE void queueCacheRequest (utils::ExpandableTickRange affectedRange)
  {
    pending_call_ = true;
    if (affected_range_.has_value ())
      {
        affected_range_->expand (affectedRange);
      }
    else
      {
        affected_range_.emplace (affectedRange);
      }

    // If already pending, the timer will reset with the new delay
    timer_.start (delay_);
  }

  /**
   * @brief Sets the delay to be used starting from the next cache request.
   */
  void setDelay (std::chrono::milliseconds delay) { delay_ = delay; }

Q_SIGNALS:
  void cacheRequested (utils::ExpandableTickRange affectedRange);

private Q_SLOTS:
  void executePendingCall ()
  {
    if (pending_call_)
      {
        pending_call_ = false;
        Q_EMIT cacheRequested (affected_range_.value ());
        affected_range_.reset ();
      }
  }

private:
  QTimer                                    timer_;
  std::chrono::milliseconds                 delay_{ 100ms };
  bool                                      pending_call_{ false };
  std::optional<utils::ExpandableTickRange> affected_range_;
};
}
