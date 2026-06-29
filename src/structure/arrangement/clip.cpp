// SPDX-FileCopyrightText: (C) 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <stdexcept>

#include "structure/arrangement/clip.h"

#include <au/math.hh>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{

Clip::Clip (
  Type                        type,
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  QObject *                   parent) noexcept
    : ArrangerObject (
        type,
        tempo_map_wrapper,
        ArrangerObjectFeatures::Bounds | ArrangerObjectFeatures::Name
          | ArrangerObjectFeatures::Mute | ArrangerObjectFeatures::Color,
        parent)
{
  // ContentTimeWarp — needs position() and length() from ArrangerObject
  content_warp_ = utils::make_qobject_unique<dsp::ContentTimeWarp> (
    get_tempo_map_wrapper (),
    qobject_cast<dsp::TimelinePosition *> (position ()), length (), this);

  // Direct length changes and warp map changes → timelineLengthTicksChanged
  QObject::connect (
    length (), &dsp::Position::positionChanged, this,
    &Clip::timelineLengthTicksChanged);
  QObject::connect (
    content_warp_.get (), &dsp::ContentTimeWarp::mapChanged, this,
    &Clip::timelineLengthTicksChanged);
  // Also relay to propertiesChanged
  QObject::connect (
    content_warp_.get (), &dsp::ContentTimeWarp::mapChanged, this,
    &ArrangerObject::propertiesChanged);

  // TimebaseProvider
  timebase_provider_ = utils::make_qobject_unique<dsp::TimebaseProvider> (this);

  // ========================================================================
  // Loop range positions (merged from ArrangerObjectLoopRange)
  // ========================================================================

  clip_start_pos_ = utils::make_qobject_unique<dsp::ContentPosition> (
    [this] (units::precise_tick_t ticks) {
      ticks = min (
        ticks, units::ticks (loop_end_pos_->ticks ()) - units::ticks (-1.0));
      ticks = max (ticks, units::ticks (0.0));
      return ticks;
    },
    this);

  loop_start_pos_ = utils::make_qobject_unique<dsp::ContentPosition> (
    [this] (units::precise_tick_t ticks) {
      ticks = max (ticks, units::ticks (0.0));
      ticks = min (
        ticks, units::ticks (loop_end_pos_->ticks ()) - units::ticks (1.0));
      return ticks;
    },
    this);

  loop_end_pos_ = utils::make_qobject_unique<dsp::ContentPosition> (
    [this] (units::precise_tick_t ticks) {
      ticks = max (
        ticks, units::ticks (loop_start_pos_->ticks ()) + units::ticks (1.0));
      ticks = max (
        ticks, units::ticks (clip_start_pos_->ticks ()) + units::ticks (1.0));
      ticks = max (ticks, units::ticks (0.0));
      return ticks;
    },
    this);

  // trackBounds → update loop positions when length changes
  QObject::connect (this, &Clip::trackBoundsChanged, this, [this] (bool track) {
    if (track && !track_bounds_connection_.has_value ())
      {
        track_bounds_connection_ = QObject::connect (
          length (), &dsp::Position::positionChanged, this, [this] () {
            clipStartPosition ()->setTicks (0.0);
            loopStartPosition ()->setTicks (0.0);
            loopEndPosition ()->setTicks (length ()->ticks ());
          });

        // also emit the signal to update all positions since we are
        // now tracking the bounds
        Q_EMIT length ()->positionChanged ();
      }
    else if (!track && track_bounds_connection_.has_value ())
      {
        QObject::disconnect (track_bounds_connection_.value ());
        track_bounds_connection_.reset ();
      }
  });

  // looped signal
  QObject::connect (
    loopStartPosition (), &dsp::Position::positionChanged, this,
    &Clip::loopedChanged);
  QObject::connect (
    loopEndPosition (), &dsp::Position::positionChanged, this,
    &Clip::loopedChanged);
  QObject::connect (
    clipStartPosition (), &dsp::Position::positionChanged, this,
    &Clip::loopedChanged);
  QObject::connect (
    length (), &dsp::Position::positionChanged, this, &Clip::loopedChanged);

  // loopablePropertiesChanged — aggregate signal for any loop property change
  QObject::connect (
    this, &Clip::loopedChanged, this, &Clip::loopablePropertiesChanged);
  QObject::connect (
    this, &Clip::trackBoundsChanged, this, &Clip::loopablePropertiesChanged);

  Q_EMIT trackBoundsChanged (track_bounds_);

  // propertiesChanged relays
  QObject::connect (
    this, &Clip::trackBoundsChanged, this, &ArrangerObject::propertiesChanged);
  QObject::connect (
    loopStartPosition (), &dsp::Position::positionChanged, this,
    &ArrangerObject::propertiesChanged);
  QObject::connect (
    loopEndPosition (), &dsp::Position::positionChanged, this,
    &ArrangerObject::propertiesChanged);
  QObject::connect (
    clipStartPosition (), &dsp::Position::positionChanged, this,
    &ArrangerObject::propertiesChanged);
  QObject::connect (
    this, &Clip::loopedChanged, this, &ArrangerObject::propertiesChanged);
}

Clip::~Clip () noexcept = default;

// ========================================================================
// Bounds methods (formerly ArrangerObjectBounds)
// ========================================================================

units::sample_t
Clip::get_end_position_samples (bool end_position_inclusive) const
{
  const auto end_samples =
    content_warp_->contentToTimelineSamples (length ()->asTick ());
  return end_samples + units::samples ((end_position_inclusive ? 0 : -1));
}

units::sample_t
Clip::get_sample_duration () const
{
  return get_end_position_samples (true)
         - get_tempo_map ().tick_to_samples_rounded (position ()->asTick ());
}

double
Clip::timelineLengthTicks () const
{
  const auto end_ticks = content_warp_->contentToTimeline (length ()->asTick ());
  const auto * pos = qobject_cast<const dsp::TimelinePosition *> (position ());
  const auto   start_ticks = pos->asTick ();
  return (end_ticks - start_ticks).asDouble ();
}

bool
Clip::is_hit (const units::sample_t frames, bool object_end_pos_inclusive) const
{
  const auto * tl_pos =
    qobject_cast<const dsp::TimelinePosition *> (position ());
  const auto obj_start =
    get_tempo_map ().tick_to_samples_rounded (tl_pos->asTick ());
  const auto obj_end = get_end_position_samples (object_end_pos_inclusive);
  return obj_start <= frames && obj_end >= frames;
}

bool
Clip::is_hit_by_range (
  std::pair<units::sample_t, units::sample_t> global_frames,
  bool                                        range_start_inclusive,
  bool                                        range_end_inclusive,
  bool                                        object_end_pos_inclusive) const
{
  const auto range_start =
    range_start_inclusive
      ? global_frames.first
      : global_frames.first + units::samples (1);
  const auto range_end =
    range_end_inclusive
      ? global_frames.second
      : global_frames.second - units::samples (1);
  const auto * tl_pos =
    qobject_cast<const dsp::TimelinePosition *> (position ());
  const auto obj_start =
    get_tempo_map ().tick_to_samples_rounded (tl_pos->asTick ());
  const auto obj_end = get_end_position_samples (object_end_pos_inclusive);

  if (range_start > range_end)
    return false;

  if (obj_start >= range_start && obj_start <= range_end)
    return true;

  if (obj_end >= range_start && obj_end <= range_end)
    return true;

  if (range_start >= obj_start && range_start <= obj_end)
    return true;

  return (range_end >= obj_start && range_end <= obj_end);
}

// ========================================================================
// Loop methods (formerly ArrangerObjectLoopRange)
// ========================================================================

void
Clip::setTrackBounds (bool track)
{
  if (track_bounds_ != track)
    {
      track_bounds_ = track;
      Q_EMIT trackBoundsChanged (track);
    }
}

int
Clip::get_num_loops (bool count_incomplete) const
{
  const auto full_size = length ()->asTick ();
  const auto loop_start =
    loopStartPosition ()->asTick () - clipStartPosition ()->asTick ();
  const auto loop_size = get_loop_length_in_ticks ();
  if (loop_size.asDouble () <= 0.0) [[unlikely]]
    return 0;
  const auto span = full_size - loop_start;
  const auto full_loops =
    static_cast<int> (std::floor (span.asDouble () / loop_size.asDouble ()));
  constexpr double epsilon = 1e-6;
  const bool       add_one =
    count_incomplete
    && std::fmod (span.asDouble (), loop_size.asDouble ()) > epsilon;
  return full_loops + (add_one ? 1 : 0);
}

bool
Clip::looped () const
{
  constexpr auto epsilon = units::ticks (1e-6);
  const auto     len = units::ticks (length ()->ticks ());
  const auto     loop_end = units::ticks (loopEndPosition ()->ticks ());
  const auto     loop_start = units::ticks (loopStartPosition ()->ticks ());
  const auto     clip_start = units::ticks (clipStartPosition ()->ticks ());
  return loop_start > units::ticks (0.0) || clip_start > units::ticks (0.0)
         || abs (len - loop_end) > epsilon;
}

// ========================================================================
// init_from / serialization
// ========================================================================

void
init_from (Clip &obj, const Clip &other, utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
  // Loop positions
  obj.clip_start_pos_->setTicks (other.clip_start_pos_->ticks ());
  obj.loop_start_pos_->setTicks (other.loop_start_pos_->ticks ());
  obj.loop_end_pos_->setTicks (other.loop_end_pos_->ticks ());
  // Use the setter so the length-tracking connection is connected/disconnected
  // to match the cloned flag (the constructor left it in the `true` state).
  obj.setTrackBounds (other.track_bounds_);
  // Timebase override (mirror Track::init_from / Track::from_json pattern)
  if (other.timebase_provider_->hasOverride ())
    obj.timebase_provider_->setOverride (
      *other.timebase_provider_->overrideValue ());
}

void
to_json (nlohmann::json &j, const Clip &clip)
{
  to_json (j, static_cast<const ArrangerObject &> (clip));
  // Timebase override (if present)
  if (clip.timebase_provider_ && clip.timebase_provider_->hasOverride ())
    j[Clip::kTimebaseKey] = *clip.timebase_provider_->overrideValue ();
  // Loop range (flat keys, matching previous format)
  j[Clip::kClipStartPosKey] = *clip.clip_start_pos_;
  j[Clip::kLoopStartPosKey] = *clip.loop_start_pos_;
  j[Clip::kLoopEndPosKey] = *clip.loop_end_pos_;
  j[Clip::kTrackBoundsKey] = clip.track_bounds_;
}

void
from_json (const nlohmann::json &j, Clip &clip)
{
  from_json (j, static_cast<ArrangerObject &> (clip));
  if (j.contains (Clip::kTimebaseKey))
    {
      dsp::Timebase tb{};
      j.at (Clip::kTimebaseKey).get_to (tb);
      clip.timebase_provider_->setOverride (tb);
    }
  j.at (Clip::kClipStartPosKey).get_to (*clip.clip_start_pos_);
  j.at (Clip::kLoopStartPosKey).get_to (*clip.loop_start_pos_);
  j.at (Clip::kLoopEndPosKey).get_to (*clip.loop_end_pos_);
  j.at (Clip::kTrackBoundsKey).get_to (clip.track_bounds_);
  Q_EMIT clip.trackBoundsChanged (clip.track_bounds_);
}

} // namespace zrythm::structure::arrangement
