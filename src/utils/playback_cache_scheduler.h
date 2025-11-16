// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>

#include "utils/debouncer.h"
#include "utils/expandable_tick_range.h"
#include "utils/qt.h"

using namespace std::chrono_literals;

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
  PlaybackCacheScheduler (QObject * parent = nullptr);

  Q_INVOKABLE void
  queueCacheRequestForRange (double affectedTickStart, double affectedTickEnd);

  Q_INVOKABLE void queueFullCacheRequest ();

  Q_INVOKABLE void queueCacheRequest (utils::ExpandableTickRange affectedRange);

  /**
   * @brief Sets the delay to be used starting from the next cache request.
   */
  void setDelay (std::chrono::milliseconds delay);

Q_SIGNALS:
  void cacheRequested (utils::ExpandableTickRange affectedRange);

private:
  void execute_pending_request ();

private:
  utils::QObjectUniquePtr<utils::Debouncer> debouncer_;
  std::optional<utils::ExpandableTickRange> affected_range_;
};
}
