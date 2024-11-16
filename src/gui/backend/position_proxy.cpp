#include "common/utils/math.h"

#include "position_proxy.h"
#include "realtime_updater.h"

PositionProxy::PositionProxy (
  QObject *        parent,
  const Position * pos,
  bool             realtime_updateable)
    : QObject (parent), Position ((pos != nullptr) ? *pos : Position ()),
      realtime_updateable_ (realtime_updateable)
{
  if (realtime_updateable_)
    {
      RealtimeUpdater::instance ().registerObject (this);
    }
}

PositionProxy::~PositionProxy ()
{
  if (realtime_updateable_)
    {
      RealtimeUpdater::instance ().deregisterObject (this);
    }
}

bool
PositionProxy::processUpdates ()
{
  bool has_updates = true;
  if (has_update_.compare_exchange_weak (has_updates, false))
    {
      Q_EMIT framesChanged ();
      Q_EMIT ticksChanged ();
      return true;
    }
  return false;
}

void
PositionProxy::setFrames (signed_frame_t frames)
{
  if (frames_ == frames)
    {
      return;
    }

  from_frames (frames);
  Q_EMIT framesChanged ();
}

void
PositionProxy::setTicks (double ticks)
{
  if (math_doubles_equal (ticks_, ticks))
    {
      return;
    }

  from_ticks (ticks);
  Q_EMIT ticksChanged ();
}

QString
PositionProxy::getStringDisplay (
  const Transport *  transport,
  const TempoTrack * tempo_track) const
{
  return QString::fromStdString (to_string (transport, tempo_track, 0));
}

void
PositionProxy::init_after_cloning (const PositionProxy &other)
{
  static_cast<Position &> (*this) = static_cast<const Position &> (other);
  realtime_updateable_ = other.realtime_updateable_;
  if (realtime_updateable_)
    {
      RealtimeUpdater::instance ().registerObject (this);
    }
}