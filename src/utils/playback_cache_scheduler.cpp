// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/playback_cache_scheduler.h"

using namespace std::chrono_literals;

namespace zrythm::utils
{

PlaybackCacheScheduler::PlaybackCacheScheduler (QObject * parent)
    : QObject (parent),
      debouncer_ (
        new utils::
          Debouncer (100ms, [this] () { execute_pending_request (); }, this))
{
}

void
PlaybackCacheScheduler::queueCacheRequestForRange (
  double affectedTickStart,
  double affectedTickEnd)
{
  queueCacheRequest ({ std::make_pair (affectedTickStart, affectedTickEnd) });
}

void
PlaybackCacheScheduler::queueFullCacheRequest ()
{
  queueCacheRequest ({});
}

void
PlaybackCacheScheduler::queueCacheRequest (
  utils::ExpandableTickRange affectedRange)
{
  if (affected_range_.has_value ())
    {
      affected_range_->expand (affectedRange);
    }
  else
    {
      affected_range_.emplace (affectedRange);
    }

  // Trigger the debouncer
  debouncer_->debounce ();
}

void
PlaybackCacheScheduler::setDelay (std::chrono::milliseconds delay)
{
  debouncer_->set_delay (delay);
}

void
PlaybackCacheScheduler::execute_pending_request ()
{
  if (affected_range_.has_value ())
    {
      Q_EMIT cacheRequested (*affected_range_);
      affected_range_.reset ();
    }
}

} // namespace zrythm::utils
