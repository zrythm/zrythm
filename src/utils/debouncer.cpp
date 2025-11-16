// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/debouncer.h"

namespace zrythm::utils
{

Debouncer::Debouncer (
  std::chrono::milliseconds delay,
  Callback                  callback,
  QObject *                 parent)
    : QObject (parent), timer_ (new QTimer (this)), delay_ (delay),
      callback_ (std::move (callback))
{
  timer_->setSingleShot (true);
  connect (timer_.get (), &QTimer::timeout, this, &Debouncer::execute_pending);
}

void
Debouncer::operator() ()
{
  pending_ = true;
  timer_->start (delay_);
}

void
Debouncer::debounce ()
{
  operator() ();
}

void
Debouncer::set_delay (std::chrono::milliseconds delay)
{
  delay_ = delay;
}

std::chrono::milliseconds
Debouncer::get_delay () const
{
  return delay_;
}

bool
Debouncer::is_pending () const
{
  return pending_;
}

void
Debouncer::cancel ()
{
  timer_->stop ();
  pending_ = false;
}

void
Debouncer::execute_pending ()
{
  if (pending_)
    {
      pending_ = false;
      callback_ ();
    }
}

} // namespace zrythm::utils
