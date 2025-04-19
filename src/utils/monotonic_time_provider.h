// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

namespace zrythm::utils
{
using MonotonicTime = qint64;

class MonotonicTimeProvider
{
public:
  virtual ~MonotonicTimeProvider () = default;
  virtual MonotonicTime get_monotonic_time_nsecs () const = 0;
  virtual MonotonicTime get_monotonic_time_usecs () const = 0;
};

class QElapsedTimeProvider : public MonotonicTimeProvider
{
public:
  QElapsedTimeProvider () { timer_.start (); }

  MonotonicTime get_monotonic_time_nsecs () const override
  {
    return timer_.nsecsElapsed ();
  }

  MonotonicTime get_monotonic_time_usecs () const override
  {
    return get_monotonic_time_nsecs () / 1000;
  }

private:
  QElapsedTimer timer_;
};

} // namespace zrythm::utils
