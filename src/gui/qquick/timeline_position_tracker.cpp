// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/content_time_warp.h"
#include "dsp/position.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/clip.h"

#include "timeline_position_tracker.h"

namespace zrythm::gui::qquick
{

TimelinePositionTracker::TimelinePositionTracker (QObject * parent)
    : QObject (parent)
{
}

void
TimelinePositionTracker::setArrangerObject (
  structure::arrangement::ArrangerObject * obj)
{
  if (obj_ == obj)
    return;

  disconnect_all ();
  obj_ = obj;
  reconnect ();

  Q_EMIT arrangerObjectChanged ();
  Q_EMIT timelineTicksChanged ();
  Q_EMIT timelineEndTicksChanged ();
}

double
TimelinePositionTracker::timelineTicks () const
{
  if (obj_ == nullptr)
    return 0.0;
  return structure::arrangement::timeline_ticks (*obj_).asDouble ();
}

double
TimelinePositionTracker::timelineEndTicks () const
{
  if (obj_ == nullptr)
    return 0.0;
  return structure::arrangement::timeline_end_ticks (*obj_).asDouble ();
}

void
TimelinePositionTracker::disconnect_all ()
{
  for (const auto &conn : connections_)
    QObject::disconnect (conn);
  connections_.clear ();
}

void
TimelinePositionTracker::reconnect ()
{
  if (obj_ == nullptr)
    return;

  // Own position changes affect both start and end
  if (auto * pos = obj_->position ())
    {
      connections_.push_back (
        QObject::connect (pos, &dsp::Position::positionChanged, this, [this] () {
          Q_EMIT timelineTicksChanged ();
          Q_EMIT timelineEndTicksChanged ();
        }));
    }

  // Own length changes affect only end
  if (auto * len = obj_->length ())
    {
      connections_.push_back (
        QObject::connect (len, &dsp::Position::positionChanged, this, [this] () {
          Q_EMIT timelineEndTicksChanged ();
        }));
    }

  // When tracking a clip itself, its own warp map affects its end position.
  if (
    const auto * self_clip =
      qobject_cast<const structure::arrangement::Clip *> (obj_))
    {
      if (auto * warp = self_clip->contentWarp ())
        {
          connections_.push_back (
            QObject::connect (
              warp, &dsp::ContentTimeWarp::mapChanged, this,
              [this] () { Q_EMIT timelineEndTicksChanged (); }));
        }
    }

  // Parent clip: warp map, length and position changes affect both start and
  // end (a child's timeline position is derived from the parent clip).
  const auto * parent = obj_->parentObject ();
  if (parent != nullptr)
    {
      if (
        const auto * clip =
          qobject_cast<const structure::arrangement::Clip *> (parent))
        {
          if (auto * warp = clip->contentWarp ())
            {
              connections_.push_back (
                QObject::connect (
                  warp, &dsp::ContentTimeWarp::mapChanged, this, [this] () {
                    Q_EMIT timelineTicksChanged ();
                    Q_EMIT timelineEndTicksChanged ();
                  }));
            }
          if (auto * clip_len = clip->length ())
            {
              connections_.push_back (
                QObject::connect (
                  clip_len, &dsp::Position::positionChanged, this, [this] () {
                    Q_EMIT timelineTicksChanged ();
                    Q_EMIT timelineEndTicksChanged ();
                  }));
            }
          if (auto * clip_pos = clip->position ())
            {
              connections_.push_back (
                QObject::connect (
                  clip_pos, &dsp::Position::positionChanged, this, [this] () {
                    Q_EMIT timelineTicksChanged ();
                    Q_EMIT timelineEndTicksChanged ();
                  }));
            }
        }
    }
}

} // namespace zrythm::gui::qquick
