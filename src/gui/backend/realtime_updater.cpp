// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/logger.h"

#include "realtime_property.h"
#include "realtime_updater.h"

RealtimeUpdater::RealtimeUpdater (QObject * parent) : QObject (parent)
{
  timer_.setInterval (16); // 60Hz updates
  connect (&timer_, &QTimer::timeout, this, &RealtimeUpdater::processUpdates);
  timer_.start ();
}

void
RealtimeUpdater::processUpdates ()
{
  QMutexLocker lock (&mutex_);
  for (auto * obj : objects_)
    {
      auto * rt_prop = dynamic_cast<IRealtimeProperty *> (obj);
      z_return_if_fail (rt_prop);
      rt_prop->processUpdates ();
    }
}

void
RealtimeUpdater::registerObject (QObject * obj)
{
  QMutexLocker lock (&mutex_);
  objects_.push_back (obj);
}

void
RealtimeUpdater::deregisterObject (QObject * obj)
{
  QMutexLocker lock (&mutex_);
  objects_.removeOne (obj);
}

RealtimeUpdater::~RealtimeUpdater ()
{
  timer_.stop ();
}