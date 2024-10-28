#include "common/utils/math.h"

#include "position_proxy.h"
#include "realtime_updater.h"

PositionProxy::PositionProxy (QObject * parent, const Position * pos)
    : QObject (parent), Position (pos ? *pos : Position ())
{
  RealtimeUpdater::instance ().registerObject (this);
}

PositionProxy::~PositionProxy ()
{
  RealtimeUpdater::instance ().deregisterObject (this);
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