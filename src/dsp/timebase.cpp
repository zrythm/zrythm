// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/timebase.h"

namespace zrythm::dsp::timebase
{

TimebaseProvider::TimebaseProvider (QObject * parent) : QObject (parent) { }

void
TimebaseProvider::setSource (TimebaseProvider * source)
{
  if (source_ == source)
    return;
  // Disconnect the previously-observed source (4-arg disconnect drops all
  // matching connections, so no connection handle needs to be tracked).
  if (source_ != nullptr)
    {
      QObject::disconnect (
        source_, &TimebaseProvider::effectiveTimebaseChanged, this,
        &TimebaseProvider::recompute_and_emit_if_changed);
    }
  source_ = source;
  if (source_ != nullptr)
    {
      QObject::connect (
        source_, &TimebaseProvider::effectiveTimebaseChanged, this,
        &TimebaseProvider::recompute_and_emit_if_changed);
    }
  recompute_and_emit_if_changed ();
}

void
TimebaseProvider::setOverride (Timebase t)
{
  if (override_ == t)
    return;
  override_ = t;
  Q_EMIT overrideChanged ();
  recompute_and_emit_if_changed ();
}

void
TimebaseProvider::clearOverride ()
{
  if (!override_.has_value ())
    return;
  override_.reset ();
  Q_EMIT overrideChanged ();
  recompute_and_emit_if_changed ();
}

Timebase
TimebaseProvider::effectiveTimebase () const
{
  // Walk the source chain iteratively (bounded by kMaxChainDepth) rather than
  // recursing, so a cyclic source graph cannot overflow the stack. A chain
  // that exceeds the depth cap behaves as if it ended and yields Musical.
  const TimebaseProvider * cur = this;
  for (int depth = 0; depth < kMaxChainDepth; ++depth)
    {
      if (cur->override_.has_value ())
        return *cur->override_;
      if (cur->source_ == nullptr)
        break;
      cur = cur->source_;
    }
  return Timebase::Musical;
}

void
TimebaseProvider::recompute_and_emit_if_changed ()
{
  const auto now = effectiveTimebase ();
  if (now != cached_effective_)
    {
      cached_effective_ = now;
      Q_EMIT effectiveTimebaseChanged ();
    }
}

} // namespace zrythm::dsp::timebase
