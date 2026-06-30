// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/content_time_warp.h"
#include "dsp/tempo_map.h"

#include <au/math.hh>

namespace zrythm::dsp
{

ContentTimeWarp::ContentTimeWarp (
  const TempoMapWrapper   &tempo_map_wrapper,
  const TimelinePosition * clip_position,
  const ContentPosition *  clip_length,
  QObject *                parent)
    : QObject (parent), tempo_map_wrapper_ (tempo_map_wrapper),
      clip_position_ (clip_position), clip_length_ (clip_length)
{
}

TimelineTick
ContentTimeWarp::contentToTimeline (ContentTick content) const
{
  const auto pos_ticks =
    (clip_position_ != nullptr) ? clip_position_->asTick () : TimelineTick{};
  return pos_ticks + warp_lookup (warp_points_, content);
}

double
ContentTimeWarp::contentToTimelineTicksRelative (double contentTicks) const
{
  return warp_lookup (warp_points_, ContentTick{ units::ticks (contentTicks) })
    .asDouble ();
}

units::sample_t
ContentTimeWarp::contentToTimelineSamples (ContentTick content) const
{
  const auto timeline_ticks = contentToTimeline (content);
  return tempo_map_wrapper_.get_tempo_map ().tick_to_samples_rounded (
    timeline_ticks);
}

ContentTick
ContentTimeWarp::timelineToContent (TimelineTick timeline) const
{
  const auto pos_ticks =
    (clip_position_ != nullptr) ? clip_position_->asTick () : TimelineTick{};
  return reverse_warp_lookup (warp_points_, timeline - pos_ticks);
}

void
ContentTimeWarp::configure_as_project (units::bpm_t source_bpm)
{
  mode_ = Mode::Project;
  source_bpm_ = source_bpm;
  user_markers_.clear ();
  disconnect_all ();

  // Length signal: rebuild identity warp points when length changes.
  if (clip_length_ != nullptr)
    len_conn_ = QObject::connect (
      clip_length_, &Position::positionChanged, this, [this] () {
        rebuild ();
        Q_EMIT mapChanged ();
      });

  rebuild ();
  Q_EMIT mapChanged ();
}

void
ContentTimeWarp::configure_as_source (units::bpm_t source_bpm)
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
  units::bpm_t               source_bpm,
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

  if (clip_position_ != nullptr)
    pos_conn_ = QObject::connect (
      clip_position_, &Position::positionChanged, this, [this] () {
        rebuild ();
        Q_EMIT mapChanged ();
      });

  if (clip_length_ != nullptr)
    len_conn_ = QObject::connect (
      clip_length_, &Position::positionChanged, this, [this] () {
        rebuild ();
        Q_EMIT mapChanged ();
      });
}

void
ContentTimeWarp::rebuild ()
{
  warp_points_.clear ();

  if (clip_position_ == nullptr || clip_length_ == nullptr)
    return;

  const auto length_ticks = clip_length_->asTick ();

  if (mode_ == Mode::Project)
    {
      if (source_bpm_ > units::bpm (0.0))
        {
          // Identity warp: content ticks map 1:1 to delta ticks.
          // to_time_warp_map will derive the sample-space stretch from these
          // (source BPM != project tempo -> stretch needed).
          warp_points_.push_back (
            { .content_ticks = ContentTick{ units::ticks (0.0) },
              .timeline_delta_ticks = TimelineTick{ units::ticks (0.0) } });
          warp_points_.push_back (
            { .content_ticks = length_ticks,
              .timeline_delta_ticks =
                TimelineTick{ length_ticks.asQuantity () } });
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
        || warp_points_.front ().content_ticks.asDouble () > 0.0)
        {
          warp_points_.insert (
            warp_points_.begin (),
            { .content_ticks = ContentTick{ units::ticks (0.0) },
              .timeline_delta_ticks = TimelineTick{ units::ticks (0.0) } });
        }

      // Ensure terminal point at {length, extrapolated_delta}.
      const auto last_ct = warp_points_.back ().content_ticks;
      if (last_ct.asDouble () < length_ticks.asDouble () - 0.5)
        {
          double slope = 1.0; // identity fallback
          if (warp_points_.size () >= 2)
            {
              const auto &prev = warp_points_[warp_points_.size () - 2];
              const auto &last = warp_points_.back ();
              const auto  seg_len =
                last.content_ticks.asDouble () - prev.content_ticks.asDouble ();
              if (seg_len > 0.0)
                slope =
                  (last.timeline_delta_ticks.asDouble ()
                   - prev.timeline_delta_ticks.asDouble ())
                  / seg_len;
            }
          const auto terminal_delta = units::ticks (
            warp_points_.back ().timeline_delta_ticks.asDouble ()
            + slope * (length_ticks.asDouble () - last_ct.asDouble ()));
          warp_points_.push_back (
            { .content_ticks = length_ticks,
              .timeline_delta_ticks = TimelineTick{ terminal_delta } });
        }

      std::ranges::sort (warp_points_, {}, &WarpPoint::content_ticks);
      return;
    }

  if (source_bpm_ <= units::bpm (0.0))
    return;

  const auto &tempo_map = tempo_map_wrapper_.get_tempo_map ();

  const auto clip_start_ticks = clip_position_->asTick ();
  const auto clip_start_seconds = tempo_map.tick_to_seconds (clip_start_ticks);

  // Helper: content ticks -> timeline delta ticks via seconds.
  auto delta_for_content = [&] (ContentTick ct) -> units::precise_tick_t {
    const auto secs = clip_start_seconds + ct.asQuantity () / source_bpm_;
    return tempo_map.seconds_to_tick (secs).asQuantity ()
           - clip_start_ticks.asQuantity ();
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
  const auto     dense_stride = k_dense_cadence_secs * source_bpm_;

  // Build segment boundaries with curve types.
  struct SegBoundary
  {
    ContentTick         content_ticks;
    TempoMap::CurveType curve;
  };

  TempoMap::CurveType current_curve = TempoMap::CurveType::Constant;
  for (const auto &ev : tempo_map.tempo_events ())
    {
      if (ev.tick <= clip_start_ticks.asQuantity ())
        current_curve = ev.curve;
      else
        break;
    }

  std::vector<SegBoundary> boundaries;
  boundaries.push_back (
    { .content_ticks = ContentTick{ units::ticks (0.0) },
      .curve = current_curve });

  for (const auto &event : tempo_map.tempo_events ())
    {
      if (event.tick <= clip_start_ticks.asQuantity ())
        continue;

      const auto ev_seconds =
        tempo_map.tick_to_seconds (TimelineTick{ event.tick });
      const auto ct = (ev_seconds - clip_start_seconds) * source_bpm_;
      if (ct >= length_ticks.asQuantity ())
        break;

      boundaries.push_back (
        { .content_ticks = ContentTick{ ct }, .curve = event.curve });
    }

  // Emit warp points, with dense sampling for Linear segments.
  for (size_t i = 0; i < boundaries.size (); ++i)
    {
      const auto &b = boundaries[i];
      warp_points_.push_back (
        { .content_ticks = b.content_ticks,
          .timeline_delta_ticks =
            TimelineTick{ delta_for_content (b.content_ticks) } });

      if (b.curve == TempoMap::CurveType::Linear)
        {
          const auto next_ct =
            (i + 1 < boundaries.size ())
              ? boundaries[i + 1].content_ticks
              : length_ticks;
          for (
            auto ct = b.content_ticks.asQuantity () + dense_stride;
            ct < next_ct.asQuantity () - units::ticks (0.5); ct += dense_stride)
            {
              warp_points_.push_back (
                { .content_ticks = ContentTick{ ct },
                  .timeline_delta_ticks =
                    TimelineTick{ delta_for_content (ContentTick{ ct }) } });
            }
        }
    }

  // Terminal warp point.
  warp_points_.push_back (
    { .content_ticks = length_ticks,
      .timeline_delta_ticks = TimelineTick{ delta_for_content (length_ticks) } });

  // Ensure sorted and strictly increasing.
  std::ranges::sort (warp_points_, {}, &WarpPoint::content_ticks);
}

bool
ContentTimeWarp::is_identity () const
{
  if (warp_points_.empty ())
    return true;
  return std::ranges::all_of (warp_points_, [] (const WarpPoint &wp) {
    return std::abs (
             wp.content_ticks.asDouble () - wp.timeline_delta_ticks.asDouble ())
           <= 0.5;
  });
}

TimelineTick
warp_lookup (
  std::span<const ContentTimeWarp::WarpPoint> warp_points,
  ContentTick                                 content_ticks)
{
  if (warp_points.empty ())
    return TimelineTick{ content_ticks.asQuantity () };

  if (content_ticks <= warp_points.front ().content_ticks)
    return warp_points.front ().timeline_delta_ticks;

  if (content_ticks >= warp_points.back ().content_ticks)
    {
      if (warp_points.size () < 2)
        return warp_points.back ().timeline_delta_ticks;
      const auto &prev = warp_points[warp_points.size () - 2];
      const auto &last = warp_points.back ();
      const auto  seg_len =
        last.content_ticks.asDouble () - prev.content_ticks.asDouble ();
      if (seg_len <= 0.0)
        return last.timeline_delta_ticks;
      const double slope =
        (last.timeline_delta_ticks.asDouble ()
         - prev.timeline_delta_ticks.asDouble ())
        / seg_len;
      return TimelineTick{ units::ticks (
        last.timeline_delta_ticks.asDouble ()
        + slope * (content_ticks.asDouble () - last.content_ticks.asDouble ())) };
    }

  auto upper = std::ranges::upper_bound (
    warp_points, content_ticks, {}, &ContentTimeWarp::WarpPoint::content_ticks);
  auto lower = std::prev (upper);

  const auto seg_len =
    upper->content_ticks.asDouble () - lower->content_ticks.asDouble ();
  const double t =
    seg_len > 0.0
      ? (content_ticks.asDouble () - lower->content_ticks.asDouble ()) / seg_len
      : 0.0;

  return TimelineTick{ units::ticks (
    lower->timeline_delta_ticks.asDouble ()
    + t * (upper->timeline_delta_ticks.asDouble () - lower->timeline_delta_ticks.asDouble ())) };
}

ContentTick
reverse_warp_lookup (
  std::span<const ContentTimeWarp::WarpPoint> warp_points,
  TimelineTick                                timeline_delta_ticks)
{
  if (warp_points.empty ())
    return ContentTick{ timeline_delta_ticks.asQuantity () };

  const auto delta = timeline_delta_ticks.asDouble ();

  if (delta <= warp_points.front ().timeline_delta_ticks.asDouble ())
    return warp_points.front ().content_ticks;

  if (delta >= warp_points.back ().timeline_delta_ticks.asDouble ())
    {
      if (warp_points.size () < 2)
        return warp_points.back ().content_ticks;
      const auto &prev = warp_points[warp_points.size () - 2];
      const auto &last = warp_points.back ();
      const auto  seg_len =
        last.timeline_delta_ticks.asDouble ()
        - prev.timeline_delta_ticks.asDouble ();
      if (seg_len <= 0.0)
        return last.content_ticks;
      const double slope =
        (last.content_ticks.asDouble () - prev.content_ticks.asDouble ())
        / seg_len;
      return ContentTick{ units::ticks (
        last.content_ticks.asDouble ()
        + slope * (delta - last.timeline_delta_ticks.asDouble ())) };
    }

  auto upper = std::ranges::upper_bound (
    warp_points, timeline_delta_ticks, {},
    &ContentTimeWarp::WarpPoint::timeline_delta_ticks);
  auto lower = std::prev (upper);

  const auto seg_len =
    upper->timeline_delta_ticks.asDouble ()
    - lower->timeline_delta_ticks.asDouble ();
  const double t =
    seg_len > 0.0
      ? (delta - lower->timeline_delta_ticks.asDouble ()) / seg_len
      : 0.0;

  return ContentTick{ units::ticks (
    lower->content_ticks.asDouble ()
    + t * (upper->content_ticks.asDouble () - lower->content_ticks.asDouble ())) };
}

} // namespace zrythm::dsp
