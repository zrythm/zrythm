// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/position_proxy.h"
#include "gui/backend/realtime_updater.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/tempo_track.h"
#include "utils/math.h"

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

  from_frames (frames, AUDIO_ENGINE->ticks_per_frame_);
  Q_EMIT framesChanged ();
}

void
PositionProxy::setTicks (double ticks)
{
  if (utils::math::floats_equal (ticks_, ticks))
    {
      return;
    }

  from_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
  Q_EMIT ticksChanged ();
}

QString
PositionProxy::getStringDisplay (
  const Transport *  transport,
  const TempoTrack * tempo_track) const
{
  return QString::fromStdString (to_string (
    tempo_track->get_beats_per_bar (), transport->sixteenths_per_beat_,
    transport->project_->audio_engine_->frames_per_tick_, 0));
}

void
PositionProxy::init_after_cloning (
  const PositionProxy &other,
  ObjectCloneType      clone_type)
{
  static_cast<Position &> (*this) = static_cast<const Position &> (other);
  realtime_updateable_ = other.realtime_updateable_;
  if (realtime_updateable_)
    {
      RealtimeUpdater::instance ().registerObject (this);
    }
}
