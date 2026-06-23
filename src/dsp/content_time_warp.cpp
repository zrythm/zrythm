// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/content_time_warp.h"
#include "dsp/tempo_map.h"

#include <au/math.hh>

namespace zrythm::dsp
{

ContentTimeWarp::ContentTimeWarp (
  const TempoMapWrapper           &tempo_map_wrapper,
  const AtomicPositionQmlAdapter * region_position,
  const AtomicPositionQmlAdapter * region_length,
  QObject *                        parent)
    : QObject (parent), tempo_map_wrapper_ (tempo_map_wrapper),
      region_position_ (region_position), region_length_ (region_length)
{
}

units::precise_tick_t
ContentTimeWarp::content_to_timeline_ticks (
  units::precise_tick_t content_ticks) const
{
  const auto pos_ticks =
    (region_position_ != nullptr)
      ? region_position_->position ().get_ticks ()
      : units::ticks (0.0);
  return pos_ticks + warp_lookup (warp_points_, content_ticks);
}

units::sample_t
ContentTimeWarp::content_to_timeline_samples (
  units::precise_tick_t content_ticks) const
{
  const auto timeline_ticks = content_to_timeline_ticks (content_ticks);
  return tempo_map_wrapper_.get_tempo_map ().tick_to_samples_rounded (
    timeline_ticks);
}

void
ContentTimeWarp::configure_as_project (double source_bpm)
{
  mode_ = Mode::Project;
  source_bpm_ = source_bpm;
  user_markers_.clear ();
  disconnect_all ();

  // Length signal: rebuild identity warp points when length changes.
  if (region_length_ != nullptr)
    len_conn_ = QObject::connect (
      region_length_, &AtomicPositionQmlAdapter::positionChanged, this,
      [this] () {
        rebuild ();
        Q_EMIT mapChanged ();
      });

  rebuild ();
  Q_EMIT mapChanged ();
}

void
ContentTimeWarp::configure_as_source (double source_bpm)
{
  mode_ = Mode::Source;
  source_bpm_ = source_bpm;
  user_markers_.clear ();
  connect_for_mode ();
  rebuild ();
  Q_EMIT mapChanged ();
}

void
ContentTimeWarp::configure_as_warped (
  double                     source_bpm,
  std::span<const WarpPoint> user_markers)
{
  mode_ = Mode::Warped;
  source_bpm_ = source_bpm;
  user_markers_.assign (user_markers.begin (), user_markers.end ());
  connect_for_mode ();
  rebuild ();
  Q_EMIT mapChanged ();
}

void
ContentTimeWarp::disconnect_all ()
{
  if (tempo_conn_ != nullptr)
    QObject::disconnect (tempo_conn_);
  if (pos_conn_ != nullptr)
    QObject::disconnect (pos_conn_);
  if (len_conn_ != nullptr)
    QObject::disconnect (len_conn_);
  tempo_conn_ = {};
  pos_conn_ = {};
  len_conn_ = {};
}

void
ContentTimeWarp::connect_for_mode ()
{
  disconnect_all ();

  tempo_conn_ = QObject::connect (
    &tempo_map_wrapper_, &TempoMapWrapper::tempoEventsChanged, this, [this] () {
      rebuild ();
      Q_EMIT mapChanged ();
    });

  if (region_position_ != nullptr)
    pos_conn_ = QObject::connect (
      region_position_, &AtomicPositionQmlAdapter::positionChanged, this,
      [this] () {
        rebuild ();
        Q_EMIT mapChanged ();
      });

  if (region_length_ != nullptr)
    len_conn_ = QObject::connect (
      region_length_, &AtomicPositionQmlAdapter::positionChanged, this,
      [this] () {
        rebuild ();
        Q_EMIT mapChanged ();
      });
}

void
ContentTimeWarp::rebuild ()
{
  warp_points_.clear ();

  if (region_position_ == nullptr || region_length_ == nullptr)
    return;

  const auto length_ticks = region_length_->position ().get_ticks ();

  if (mode_ == Mode::Project)
    {
      if (source_bpm_ > 0.0)
        {
          // Identity warp: content ticks map 1:1 to delta ticks.
          // to_time_warp_map will derive the sample-space stretch from these
          // (source BPM != project tempo -> stretch needed).
          warp_points_.push_back (
            { .content_ticks = units::ticks (0.0),
              .timeline_delta_ticks = units::ticks (0.0) });
          warp_points_.push_back (
            { .content_ticks = length_ticks,
              .timeline_delta_ticks = length_ticks });
        }
      return;
    }

  if (mode_ == Mode::Warped)
    {
      // Use user-supplied markers as canonical warp points.
      warp_points_ = user_markers_;

      // Ensure origin {0, 0} is present.
      if (
        warp_points_.empty ()
        || warp_points_.front ().content_ticks > units::ticks (0.0))
        {
          warp_points_.insert (
            warp_points_.begin (),
            { .content_ticks = units::ticks (0.0),
              .timeline_delta_ticks = units::ticks (0.0) });
        }

      // Ensure terminal point at {length, extrapolated_delta}.
      const auto last_ct = warp_points_.back ().content_ticks;
      if (last_ct < length_ticks - units::ticks (0.5))
        {
          double slope = 1.0; // identity fallback
          if (warp_points_.size () >= 2)
            {
              const auto &prev = warp_points_[warp_points_.size () - 2];
              const auto &last = warp_points_.back ();
              const auto  seg_len = last.content_ticks - prev.content_ticks;
              if (seg_len.in (units::ticks) > 0.0)
                slope =
                  (last.timeline_delta_ticks - prev.timeline_delta_ticks)
                    .in (units::ticks)
                  / seg_len.in (units::ticks);
            }
          const auto terminal_delta =
            warp_points_.back ().timeline_delta_ticks
            + slope * (length_ticks - last_ct);
          warp_points_.push_back (
            { .content_ticks = length_ticks,
              .timeline_delta_ticks = terminal_delta });
        }

      std::ranges::sort (warp_points_, {}, &WarpPoint::content_ticks);
      return;
    }

  if (source_bpm_ <= 0.0)
    return;

  const auto &tempo_map = tempo_map_wrapper_.get_tempo_map ();
  const auto  factor = (source_bpm_ / units::seconds (60.0)) * units::PPQ;

  const auto region_start_ticks = region_position_->position ().get_ticks ();
  const auto region_start_seconds =
    tempo_map.tick_to_seconds (region_start_ticks);

  // Helper: content ticks -> timeline delta ticks via seconds.
  auto delta_for_content =
    [&] (units::precise_tick_t ct) -> units::precise_tick_t {
    const auto secs = region_start_seconds + ct / factor;
    return tempo_map.seconds_to_tick (secs) - region_start_ticks;
  };

  // Dense sampling stride for Linear ramps (~50ms cadence).
  //
  // NOTE: This bakes a piecewise-linear approximation into the canonical warp
  // point list. Boundary points are analytically exact (via tick_to_seconds /
  // seconds_to_tick). Dense intermediate points are also exact. The only
  // approximation is warp_lookup() queries at positions between warp points,
  // which are linearly interpolated. At 50ms cadence the error is sub-sample
  // for typical tempos (60-200 BPM). Current consumers only query at endpoints
  // (exact warp points). If exact intermediate queries are needed in the future
  // (e.g., arbitrary warp markers), WarpPoint can be extended with a per-segment
  // curve tag and warp_lookup() upgraded to interpolate analytically.
  constexpr auto k_dense_cadence_secs = units::seconds (0.05);
  const auto     dense_stride = k_dense_cadence_secs * factor;

  // Build segment boundaries with curve types.
  struct SegBoundary
  {
    units::precise_tick_t content_ticks;
    TempoMap::CurveType   curve;
  };

  TempoMap::CurveType current_curve = TempoMap::CurveType::Constant;
  for (const auto &ev : tempo_map.tempo_events ())
    {
      if (ev.tick <= region_start_ticks)
        current_curve = ev.curve;
      else
        break;
    }

  std::vector<SegBoundary> boundaries;
  boundaries.push_back (
    { .content_ticks = units::ticks (0.0), .curve = current_curve });

  for (const auto &event : tempo_map.tempo_events ())
    {
      if (event.tick <= region_start_ticks)
        continue;

      const auto ev_seconds = tempo_map.tick_to_seconds (event.tick);
      const auto ct = (ev_seconds - region_start_seconds) * factor;
      if (ct >= length_ticks)
        break;

      boundaries.push_back ({ .content_ticks = ct, .curve = event.curve });
    }

  // Emit warp points, with dense sampling for Linear segments.
  for (size_t i = 0; i < boundaries.size (); ++i)
    {
      const auto &b = boundaries[i];
      warp_points_.push_back (
        { .content_ticks = b.content_ticks,
          .timeline_delta_ticks = delta_for_content (b.content_ticks) });

      if (b.curve == TempoMap::CurveType::Linear)
        {
          const auto next_ct =
            (i + 1 < boundaries.size ())
              ? boundaries[i + 1].content_ticks
              : length_ticks;
          for (
            auto ct = b.content_ticks + dense_stride;
            ct < next_ct - units::ticks (0.5); ct += dense_stride)
            {
              warp_points_.push_back (
                { .content_ticks = ct,
                  .timeline_delta_ticks = delta_for_content (ct) });
            }
        }
    }

  // Terminal warp point.
  warp_points_.push_back (
    { .content_ticks = length_ticks,
      .timeline_delta_ticks = delta_for_content (length_ticks) });

  // Ensure sorted and strictly increasing.
  std::ranges::sort (warp_points_, {}, &WarpPoint::content_ticks);
}

bool
ContentTimeWarp::is_identity () const
{
  if (warp_points_.empty ())
    return true;
  const auto epsilon = units::ticks (0.5);
  return std::ranges::all_of (warp_points_, [&epsilon] (const WarpPoint &wp) {
    return abs (wp.content_ticks - wp.timeline_delta_ticks) <= epsilon;
  });
}

units::precise_tick_t
warp_lookup (
  std::span<const ContentTimeWarp::WarpPoint> warp_points,
  units::precise_tick_t                       content_ticks)
{
  if (warp_points.empty ())
    return content_ticks;

  if (content_ticks <= warp_points.front ().content_ticks)
    return warp_points.front ().timeline_delta_ticks;

  if (content_ticks >= warp_points.back ().content_ticks)
    {
      if (warp_points.size () < 2)
        return warp_points.back ().timeline_delta_ticks;
      const auto &prev = warp_points[warp_points.size () - 2];
      const auto &last = warp_points.back ();
      const auto  seg_len = last.content_ticks - prev.content_ticks;
      if (seg_len.in (units::ticks) <= 0.0)
        return last.timeline_delta_ticks;
      const double slope =
        (last.timeline_delta_ticks - prev.timeline_delta_ticks).in (units::ticks)
        / seg_len.in (units::ticks);
      return last.timeline_delta_ticks
             + units::ticks (
               slope * (content_ticks - last.content_ticks).in (units::ticks));
    }

  auto upper = std::ranges::upper_bound (
    warp_points, content_ticks, {}, &ContentTimeWarp::WarpPoint::content_ticks);
  auto lower = std::prev (upper);

  const auto   seg_len = upper->content_ticks - lower->content_ticks;
  const double t =
    seg_len.in (units::ticks) > 0.0
      ? (content_ticks - lower->content_ticks).in (units::ticks)
          / seg_len.in (units::ticks)
      : 0.0;

  return lower->timeline_delta_ticks
         + units::ticks (
           t
           * (upper->timeline_delta_ticks - lower->timeline_delta_ticks)
               .in (units::ticks));
}

} // namespace zrythm::dsp
